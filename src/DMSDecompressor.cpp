/* Copyright (C) Teemu Suutari */

#include "DMSDecompressor.hpp"

#include "HuffmanDecoder.hpp"
#include "DynamicHuffmanDecoder.hpp"

#include <FixedMemoryBuffer.hpp>
#include <CRC16.hpp>

bool DMSDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC('DMS!');
}

std::unique_ptr<Decompressor> DMSDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<DMSDecompressor>(packedData,verify);
}

DMSDecompressor::DMSDecompressor(const Buffer &packedData,bool verify) :
	_packedData(packedData)
{
	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr) || packedData.size()<56) throw InvalidFormatError();

	if (verify && CRC16(packedData,4,50,0)!=packedData.readBE16(54))
		throw VerificationError();

	uint16_t info=packedData.readBE16(10);
	_isObsfuscated=info&2;				// using 16 bit key is not encryption, it is obsfuscation
	_isHD=info&16;
	if (info&32) throw InvalidFormatError();	// MS-DOS disk

	// packed data in header is useless, as is rawsize and track numbers
	// they are not always filled

	if (packedData.readBE16(50)>6) throw InvalidFormatError(); // either FMS or unknown

	// now calculate the real packed size, including headers
	uint32_t offset=56;
	uint32_t accountedSize=0;
	uint32_t lastTrackSize=0;
	uint32_t numTracks=0;
	uint32_t minTrack=80;
	uint32_t prevTrack=0;
	while (offset+20<packedData.size())
	{
		if (_packedData.readBE16(offset)!='TR')
		{
			// secondary exit criteria, should not be like this, if the header would be trustworthy
			if (!accountedSize) throw InvalidFormatError();
			break;
		}
		uint32_t trackNo=_packedData.readBE16(offset+2);
		// lets not go backwards on tracks!
		if (trackNo<prevTrack) break;

		// header check
		if (verify && CRC16(packedData,offset,18,0)!=packedData.readBE16(offset+18))
			throw VerificationError();

		uint8_t mode=_packedData.read8(offset+13);
		if (mode>6) throw InvalidFormatError();
		static const uint32_t contextSizes[7]={0,0,256,16384,16384,4096,8192};
		_contextBufferSize=std::max(_contextBufferSize,contextSizes[mode]);

		uint32_t packedChunkLength=packedData.readBE16(offset+6);
		if (offset+20+packedChunkLength>packedData.size())
			throw InvalidFormatError();
		if (verify && CRC16(packedData,offset+20,packedChunkLength,0)!=packedData.readBE16(offset+16))
			throw VerificationError();

		if (trackNo<80)
		{
			if (trackNo>=numTracks) lastTrackSize=_packedData.readBE16(offset+10); 
			minTrack=std::min(minTrack,trackNo);
			numTracks=std::max(numTracks,trackNo);
			prevTrack=trackNo;
		}

		offset+=20+packedChunkLength;
		accountedSize+=packedChunkLength;
		// this is the real exit critea, unfortunately
		if (trackNo>=79 && trackNo<0x8000U) break;
	}
	uint32_t trackSize=(_isHD)?22528:11264;
	_rawOffset=minTrack*trackSize;
	_rawSize=(numTracks-minTrack)*trackSize+lastTrackSize;
	_imageSize=trackSize*80;

	_packedSize=offset;
	if (_packedSize>getMaxPackedSize())
		throw InvalidFormatError();
}

DMSDecompressor::~DMSDecompressor()
{
	// nothing needed
}

const std::string &DMSDecompressor::getName() const noexcept
{
	static std::string name="DMS: Disk Masher System";
	return name;
}

size_t DMSDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t DMSDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

size_t DMSDecompressor::getImageSize() const noexcept
{
	return _imageSize;
}

size_t DMSDecompressor::getImageOffset() const noexcept
{
	return _rawOffset;
}

class HeavyDecoder : public HuffmanDecoder<uint32_t,0x10000U,0>
{
public:
	HeavyDecoder() {}
	~HeavyDecoder() {}

	void setZero(uint32_t index)
	{
		_isZeroLength=true;
		_zeroIndex=index;
	}

	template<typename F>
	uint32_t heavyDecode(F bitReader)
	{
		if (_isZeroLength) return _zeroIndex;
			else return decode(bitReader);
	}

private:
	bool		_isZeroLength=false;
	uint32_t	_zeroIndex=0;
};

void DMSDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	if (!_isObsfuscated)
	{
		if (!decompressImpl(rawData,verify,~0U,0))
			throw DecompressionError();
	} else {
		// fast try: If the disc is bootable, we can detect it really quickly
		if (!_rawOffset) for (uint32_t passCandidate=0;passCandidate<0x10000U;passCandidate++)
		{
			try
			{
				if (!decompressImpl(rawData,false,8,passCandidate)) continue;
				if ((rawData.readBE32(0)&0xffff'ff00U)!=FourCC('DOS\0')) continue;
				if (!decompressImpl(rawData,true,~0U,passCandidate)) continue;
				return;
			} catch (const Buffer::Error &) {
				// just continue
			} catch (const Decompressor::Error &) {
				// just continue
			}
		}
		// slow try
		for (uint32_t passCandidate=0;passCandidate<0x10000U;passCandidate++)
		{
			try
			{
				if (!decompressImpl(rawData,true,~0U,passCandidate)) continue;
				return;
			} catch (const Buffer::Error &) {
				// just continue
			} catch (const Decompressor::Error &) {
				// just continue
			}
		}
		throw DecompressionError();
	}
}

// Exceptions affect performance here when doing guessing loop, have a return value in addition to exceptions
// This makes the code even more messier. Like the implementatio would need any more than that
// However we can use it surgigally on the fast path...
// TODO: limitedDecompress only works on first track!
bool DMSDecompressor::decompressImpl(Buffer &rawData,bool verify,uint32_t limitedDecompress,uint16_t passCode)
{
	// fill unused tracks with zeros
	::memset(rawData.data(),0,std::min(_rawSize,limitedDecompress));

	auto checksum=[](const uint8_t *src,uint32_t srcLength)->uint16_t
	{
		uint16_t ret=0;
		for (uint32_t i=0;i<srcLength;i++) ret+=uint16_t(src[i]);
		return ret;
	};

	uint16_t passAccumulator=passCode;
	auto unObsfuscate=[&](uint8_t ch,bool obsfuscate)->uint8_t
	{
		if (!obsfuscate)
		{
			return ch;
		} else {
			uint8_t ret=ch^passAccumulator;
			passAccumulator=(passAccumulator>>1)+uint16_t(ch);
			return ret;
		}
	};

	auto unpackNone=[&](uint8_t *dest,const uint8_t *src,uint32_t destLength,uint32_t srcLength)
	{
		if (srcLength>destLength) throw DecompressionError();
		if (!_isObsfuscated)
		{
			::memcpy(dest,src,srcLength);
		} else {
			for (uint32_t i=0;i<std::min(srcLength,limitedDecompress);i++)
				dest[i]=unObsfuscate(src[i],true);
		}
	};

	bool imageGood=true;

	// same as simple
	auto unRLE=[&](uint8_t *dest,const uint8_t *src,uint32_t destLength,uint32_t srcLength,bool obsfuscate,bool lastCharMissing)->uint32_t
	{
		uint32_t srcOffset=0;
		uint32_t destOffset=0;
		// hacky, hacky, hacky
		while (srcOffset!=srcLength)
		{
			if (srcOffset>=limitedDecompress || destOffset>=limitedDecompress) return 0;
			if (lastCharMissing && srcOffset+1==srcLength)
			{
				if (destOffset+1!=destLength)
				{
					imageGood=false;
					return 0;
				}
				return 1;
			}
			uint8_t ch=unObsfuscate(src[srcOffset++],obsfuscate);
			uint32_t count=1;
			if (ch==0x90U)
			{
				if (srcOffset==srcLength) throw DecompressionError();
				if (lastCharMissing && srcOffset+1==srcLength)
				{
					// only possible way this can work
					if (destOffset+1!=destLength) throw DecompressionError();
					count=0;
				} else count=uint32_t(unObsfuscate(src[srcOffset++],obsfuscate));
				if (!count)
				{
					count=1;
				} else {
					
					if (srcOffset==srcLength) throw DecompressionError();
					if (lastCharMissing && srcOffset+1==srcLength)
					{
						if (count==0xffU || destOffset+count!=destLength) throw DecompressionError();
						return count;
					}
					ch=unObsfuscate(src[srcOffset++],obsfuscate);
				}
				if (count==0xffU)
				{
					if (srcOffset+2>srcLength) throw DecompressionError();
					if (lastCharMissing && srcOffset+2>=srcLength)
					{
						count=destLength-destOffset;
					} else {
						count=uint32_t(unObsfuscate(src[srcOffset++],obsfuscate))<<8;
						count|=uint32_t(unObsfuscate(src[srcOffset++],obsfuscate));
					}
				}
			}
			if (destOffset+count>destLength) throw DecompressionError();
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=ch;
		}
		if (destOffset!=destLength) throw DecompressionError();
		return 0;
	};

	bool doInitContext=true;
	FixedMemoryBuffer contextBuffer(_contextBufferSize);
	uint8_t *contextBufferPtr=contextBuffer.data();
	// context used is 256 bytes
	uint8_t quickContextLocation;
	// context used is 16384 bytes
	uint32_t mediumContextLocation;
	uint32_t deepContextLocation;
	// context used is 4096/8192 bytes
	uint32_t heavyContextLocation;
	DynamicHuffmanDecoder<314> deepDecoder;
	auto initContext=[&]()
	{
		if (doInitContext)
		{
			if (_contextBufferSize) ::memset(contextBuffer.data(),0,_contextBufferSize);
			quickContextLocation=251;
			mediumContextLocation=16318;
			deepContextLocation=16324;
			if (deepDecoder.getMaxFrequency()!=314)
				deepDecoder.reset();
			heavyContextLocation=0;
			doInitContext=false;
		}
	};

	// Stream reading
	const uint8_t *bufPtr;
	size_t bufOffset;
	size_t packedChunkSize;
	uint32_t bufBitsContent;
	uint8_t bufBitsLength;
	uint32_t destOffset;

	auto initStream=[&](const uint8_t *stream,uint32_t length)
	{
		bufPtr=stream;
		bufOffset=0;
		packedChunkSize=length;
		bufBitsContent=0;
		bufBitsLength=0;
	};

	auto finishStream=[&]()
	{
		if (_isObsfuscated)
		{
			while (bufOffset!=packedChunkSize)
				unObsfuscate(bufPtr[bufOffset++],true);
		}
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		while (bufBitsLength<count)
		{
			if (bufOffset>=packedChunkSize) throw ShortInputError();
			bufBitsContent<<=8;
			bufBitsContent|=unObsfuscate(bufPtr[bufOffset++],_isObsfuscated);
			bufBitsLength+=8;
		}

		bufBitsLength-=count;
		return (bufBitsContent>>bufBitsLength)&((1U<<count)-1);
	};

	auto readBit=[&]()->uint32_t
	{
		return readBits(1);
	};

	auto unpackQuick=[&](uint8_t *dest,const uint8_t *src,uint32_t destLength,uint32_t srcLength)
	{
		initStream(src,srcLength);
		initContext();

		destOffset=0;
		while (destOffset!=destLength)
		{
			if (destOffset>=limitedDecompress) return;
			if (readBits(1))
			{
				dest[destOffset++]=contextBufferPtr[quickContextLocation++]=readBits(8);
			} else {
				uint32_t count=readBits(2)+2;
				if (destOffset+count>destLength) throw DecompressionError();
				uint8_t offset=quickContextLocation-readBits(8)-1;
				for (uint32_t i=0;i<count;i++)
					dest[destOffset++]=contextBufferPtr[quickContextLocation++]=contextBufferPtr[(i+offset)&0xffU];
			}
		}
		quickContextLocation+=5;
	};

	static const uint8_t lengthTable[256]={
		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,
		3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4, 5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6, 7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,
		10,10,10,10,10,10,10,10, 11,11,11,11,11,11,11,11,
		12,12,12,12,13,13,13,13, 14,14,14,14,15,15,15,15,
		16,16,16,16,17,17,17,17, 18,18,18,18,19,19,19,19,
		20,20,20,20,21,21,21,21, 22,22,22,22,23,23,23,23,
		24,24,25,25,26,26,27,27, 28,28,29,29,30,30,31,31,
		32,32,33,33,34,34,35,35, 36,36,37,37,38,38,39,39,
		40,40,41,41,42,42,43,43, 44,44,45,45,46,46,47,47,
		48,49,50,51,52,53,54,55, 56,57,58,59,60,61,62,63};

	static const uint8_t bitLengthTable[256]={
		3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
		3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8};

	auto decodeLengthValueHalf=[&](uint8_t code)->uint32_t
	{
		return (((code<<bitLengthTable[code])|readBits(bitLengthTable[code]))&0xffU);
	};

	auto decodeLengthValueFull=[&](uint8_t code)->uint32_t
	{
		return (uint32_t(lengthTable[code])<<8)|
			uint32_t(((code<<bitLengthTable[code])|readBits(bitLengthTable[code]))&0xffU);
	};

	auto unpackMedium=[&](uint8_t *dest,const uint8_t *src,uint32_t destLength,uint32_t srcLength)
	{
		initStream(src,srcLength);
		initContext();

		destOffset=0;
		while (destOffset!=destLength)
		{
			if (destOffset>=limitedDecompress) return;
			if (readBits(1))
			{
				dest[destOffset++]=contextBufferPtr[mediumContextLocation++]=readBits(8);
				mediumContextLocation&=0x3fffU;
			} else {
				uint32_t code=readBits(8);
				uint32_t count=lengthTable[code]+3;
				if (destOffset+count>destLength) throw DecompressionError();

				uint32_t tmp=decodeLengthValueFull(decodeLengthValueHalf(code));

				uint32_t offset=mediumContextLocation-tmp-1;
				for (uint32_t i=0;i<count;i++)
				{
					dest[destOffset++]=contextBufferPtr[mediumContextLocation++]=contextBufferPtr[(i+offset)&0x3fffU];
					mediumContextLocation&=0x3fffU;
				}
			}
		}
		mediumContextLocation+=66;
		mediumContextLocation&=0x3fffU;
		finishStream();
	};

	auto unpackDeep=[&](uint8_t *dest,const uint8_t *src,uint32_t destLength,uint32_t srcLength)
	{
		initStream(src,srcLength);
		initContext();

		destOffset=0;
		while (destOffset!=destLength)
		{
			uint32_t symbol=deepDecoder.decode(readBit);
			if (deepDecoder.getMaxFrequency()==0x8000U) deepDecoder.halve();
			deepDecoder.update(symbol);
			if (symbol<256)
			{
				dest[destOffset++]=contextBufferPtr[deepContextLocation++]=symbol;
				deepContextLocation&=0x3fffU;
			} else {
				uint32_t count=symbol-253;	// minimum repeat is 3
				if (destOffset+count>destLength) throw DecompressionError();
				uint32_t offset=deepContextLocation-decodeLengthValueFull(readBits(8))-1;

				for (uint32_t i=0;i<count;i++)
				{
					dest[destOffset++]=contextBufferPtr[deepContextLocation++]=contextBufferPtr[(i+offset)&0x3fffU];
					deepContextLocation&=0x3fffU;
				}
			}
		}
		deepContextLocation+=60;
		deepContextLocation&=0x3fffU;
		finishStream();
	};

	// these are not part of the initContext like other methods
	HeavyDecoder symbolDecoder,offsetDecoder;
	bool heavyLastInitialized=false;		// this is part of initContext on some implementations. screwy!!!
	uint32_t heavyLastOffset;
	auto unpackHeavy=[&](uint8_t *dest,const uint8_t *src,uint32_t destLength,uint32_t srcLength,bool initTables,bool use8kDict)
	{
		initStream(src,srcLength);
		initContext();
		// well, this works. Why this works? dunno
		if (!heavyLastInitialized)
		{
			heavyLastOffset=use8kDict?0U:~0U;
			heavyLastInitialized=true;
		}

		auto readTable=[&](HeavyDecoder &decoder,uint32_t countBits,uint32_t valueBits)
		{
			decoder.reset();
			uint32_t count=readBits(countBits);
			if (count)
			{
				FixedMemoryBuffer lengthBuffer(count);
				// in order to speed up the deObsfuscation, do not send the hopeless
				// data into slow CreateOrderlyHuffmanTable
				uint64_t sum=0;
				for (uint32_t i=0;i<count;i++)
				{
					uint32_t bits=readBits(valueBits);
					if (bits)
					{
						sum+=uint64_t(1U)<<(32-bits);
						if (sum>(uint64_t(1U)<<32))
						{
							// this is the number 1 offender for lots of exceptions
							imageGood=false;
							return;
						}
					}
					lengthBuffer[i]=bits;
				}
				CreateOrderlyHuffmanTable(decoder,lengthBuffer.data(),count);
			} else {
				uint32_t index=readBits(countBits);
				decoder.setZero(index);
			}
		};

		if (initTables)
		{
			readTable(symbolDecoder,9,5);
			if (!imageGood) return;
			readTable(offsetDecoder,5,4);
			if (!imageGood) return;
		}

		uint32_t mask=use8kDict?0x1fffU:0xfffU;
		uint32_t bitLength=use8kDict?14:13;

		destOffset=0;
		while (destOffset!=destLength)
		{
			if (destOffset>=limitedDecompress) return;
			uint32_t symbol=symbolDecoder.heavyDecode(readBit);
			if (symbol<256)
			{
				dest[destOffset++]=contextBufferPtr[heavyContextLocation++]=symbol;
				heavyContextLocation&=mask;
			} else {
				uint32_t count=symbol-253;	// minimum repeat is 3
				if (destOffset+count>destLength) throw DecompressionError();
				uint32_t offsetLength=offsetDecoder.heavyDecode(readBit);
				uint32_t rawOffset=heavyLastOffset;
				if (offsetLength!=bitLength)
				{
					if (offsetLength) rawOffset=(1<<(offsetLength-1))|readBits(offsetLength-1);
						else rawOffset=0;
					heavyLastOffset=rawOffset;
				}
				uint32_t offset=heavyContextLocation-rawOffset-1;
				for (uint32_t i=0;i<count;i++)
				{
					dest[destOffset++]=contextBufferPtr[heavyContextLocation++]=contextBufferPtr[(i+offset)&mask];
					heavyContextLocation&=mask;
				}
			}
		}
		finishStream();
	};


	uint32_t trackLength=(_isHD)?22528:11264;
	for (uint32_t packedOffset=56,packedChunkLength=0;packedOffset!=_packedSize;packedOffset+=20+packedChunkLength)
	{
		// There are some info tracks, at -1 or 80. ignore those (if still present)
		uint16_t trackNo=_packedData.readBE16(packedOffset+2);
		packedChunkLength=_packedData.readBE16(packedOffset+6);
		if (trackNo==80) break;							// should not happen, this is already excluded
		// even though only -1 should be used I've seen -2 as well. ignore all negatives
		uint32_t tmpChunkLength=_packedData.readBE16(packedOffset+8);		// after the first unpack (if twostage)
		uint32_t rawChunkLength=_packedData.readBE16(packedOffset+10);		// after final unpack
		uint8_t flags=_packedData.read8(packedOffset+12);
		uint8_t mode=_packedData.read8(packedOffset+13);
		const uint8_t *src=&_packedData[packedOffset+20];

		// could affect context, but in practice they are separate, even though there is no explicit reset
		// deal with decompression though...
		if (trackNo>=0x8000U)
		{
			if (_isObsfuscated)
			{
				for (uint32_t i=0;i<packedChunkLength;i++)
					unObsfuscate(src[i],true);
			}
			continue;
		}
		if (rawChunkLength>trackLength) throw DecompressionError();
		if (trackNo>80) throw DecompressionError();				// should not happen, already excluded

		uint32_t dataOffset=trackNo*trackLength;
		if (_rawOffset>dataOffset) throw DecompressionError();
		uint8_t *dest=&rawData[dataOffset-_rawOffset];

		// this is screwy, but it is, what it is
		auto processBlock=[&](bool doRLE,auto func,auto&&... params)
		{
			if (doRLE)
			{
				FixedMemoryBuffer tmpBuffer(tmpChunkLength);
				try
				{
					func(tmpBuffer.data(),src,tmpChunkLength,packedChunkLength,params...);
					unRLE(dest,tmpBuffer.data(),rawChunkLength,tmpChunkLength,false,false);
				} catch (const ShortInputError &) {
					// if this error happens on repeat/offset instead of char, though luck :(
					// (ditto in obsfuscated file, because that would result too much guessing)
					if (destOffset+1!=tmpChunkLength || _isObsfuscated) throw DecompressionError();
					// missing last char on src we can fix :)
					uint32_t missingNo=unRLE(dest,tmpBuffer.data(),rawChunkLength,tmpChunkLength,false,true);
					if (missingNo)
					{
						uint32_t protoSum=checksum(dest,rawChunkLength-missingNo);
						uint32_t fileSum=_packedData.readBE16(packedOffset+14);
						uint8_t ch=((fileSum>=protoSum)?fileSum-protoSum:(0x10000U+fileSum-protoSum))/missingNo;
						::memset(dest+rawChunkLength-missingNo,ch,missingNo);
					}
				}
			} else {
				try
				{
					func(dest,src,rawChunkLength,packedChunkLength,params...);
				} catch (const ShortInputError &) {
					// same deal
					if (destOffset+1!=rawChunkLength || _isObsfuscated) throw DecompressionError();
					uint32_t protoSum=checksum(dest,rawChunkLength-1);
					uint32_t fileSum=_packedData.readBE16(packedOffset+14);
					uint8_t ch=fileSum-protoSum;
					dest[rawChunkLength-1]=ch;
				}
			}
		};

		switch (mode)
		{
			case 0:
			processBlock(false,unpackNone);
			rawChunkLength=packedChunkLength;
			break;

			case 1:
			processBlock(false,unRLE,_isObsfuscated,false);
			break;

			case 2:
			processBlock(true,unpackQuick);
			break;

			case 3:
			processBlock(true,unpackMedium);
			break;

			case 4:
			processBlock(true,unpackDeep);
			break;

			// heavy flags:
			// 2: (re-)initialize/read tables
			// 4: do RLE
			// heavy1 uses 4k dictionary (mode 5), whereas heavy2 uses 8k dictionary
			case 5:
			case 6:
			processBlock(flags&4,unpackHeavy,flags&2,mode==6);
			break;

			default:
			throw DecompressionError();
		}
		if (!imageGood) break;
		if (!(flags&1)) doInitContext=true;

		if (!trackNo && limitedDecompress!=~0U) break;

		if (verify && checksum(dest,rawChunkLength)!=_packedData.readBE16(packedOffset+14))
			throw VerificationError();
	}
	return imageGood;
}

Decompressor::Registry<DMSDecompressor> DMSDecompressor::_registration;
