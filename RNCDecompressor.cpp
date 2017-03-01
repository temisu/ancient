/* Copyright (C) Teemu Suutari */

#include <algorithm>

#include "RNCDecompressor.hpp"
#include "HuffmanDecoder.hpp"

static bool RNCCRC(const Buffer &buffer,size_t offset,size_t len,uint16_t &retValue)
{
	// bit reversed 16bit CRC with 0x8005 polynomial
	static const uint16_t CRCTable[256]={
		0x0000,0xc0c1,0xc181,0x0140,0xc301,0x03c0,0x0280,0xc241,0xc601,0x06c0,0x0780,0xc741,0x0500,0xc5c1,0xc481,0x0440,
		0xcc01,0x0cc0,0x0d80,0xcd41,0x0f00,0xcfc1,0xce81,0x0e40,0x0a00,0xcac1,0xcb81,0x0b40,0xc901,0x09c0,0x0880,0xc841,
		0xd801,0x18c0,0x1980,0xd941,0x1b00,0xdbc1,0xda81,0x1a40,0x1e00,0xdec1,0xdf81,0x1f40,0xdd01,0x1dc0,0x1c80,0xdc41,
		0x1400,0xd4c1,0xd581,0x1540,0xd701,0x17c0,0x1680,0xd641,0xd201,0x12c0,0x1380,0xd341,0x1100,0xd1c1,0xd081,0x1040,
		0xf001,0x30c0,0x3180,0xf141,0x3300,0xf3c1,0xf281,0x3240,0x3600,0xf6c1,0xf781,0x3740,0xf501,0x35c0,0x3480,0xf441,
		0x3c00,0xfcc1,0xfd81,0x3d40,0xff01,0x3fc0,0x3e80,0xfe41,0xfa01,0x3ac0,0x3b80,0xfb41,0x3900,0xf9c1,0xf881,0x3840,
		0x2800,0xe8c1,0xe981,0x2940,0xeb01,0x2bc0,0x2a80,0xea41,0xee01,0x2ec0,0x2f80,0xef41,0x2d00,0xedc1,0xec81,0x2c40,
		0xe401,0x24c0,0x2580,0xe541,0x2700,0xe7c1,0xe681,0x2640,0x2200,0xe2c1,0xe381,0x2340,0xe101,0x21c0,0x2080,0xe041,
		0xa001,0x60c0,0x6180,0xa141,0x6300,0xa3c1,0xa281,0x6240,0x6600,0xa6c1,0xa781,0x6740,0xa501,0x65c0,0x6480,0xa441,
		0x6c00,0xacc1,0xad81,0x6d40,0xaf01,0x6fc0,0x6e80,0xae41,0xaa01,0x6ac0,0x6b80,0xab41,0x6900,0xa9c1,0xa881,0x6840,
		0x7800,0xb8c1,0xb981,0x7940,0xbb01,0x7bc0,0x7a80,0xba41,0xbe01,0x7ec0,0x7f80,0xbf41,0x7d00,0xbdc1,0xbc81,0x7c40,
		0xb401,0x74c0,0x7580,0xb541,0x7700,0xb7c1,0xb681,0x7640,0x7200,0xb2c1,0xb381,0x7340,0xb101,0x71c0,0x7080,0xb041,
		0x5000,0x90c1,0x9181,0x5140,0x9301,0x53c0,0x5280,0x9241,0x9601,0x56c0,0x5780,0x9741,0x5500,0x95c1,0x9481,0x5440,
		0x9c01,0x5cc0,0x5d80,0x9d41,0x5f00,0x9fc1,0x9e81,0x5e40,0x5a00,0x9ac1,0x9b81,0x5b40,0x9901,0x59c0,0x5880,0x9841,
		0x8801,0x48c0,0x4980,0x8941,0x4b00,0x8bc1,0x8a81,0x4a40,0x4e00,0x8ec1,0x8f81,0x4f40,0x8d01,0x4dc0,0x4c80,0x8c41,
		0x4400,0x84c1,0x8581,0x4540,0x8701,0x47c0,0x4680,0x8641,0x8201,0x42c0,0x4380,0x8341,0x4100,0x81c1,0x8081,0x4040};

	if (!len || offset+len>buffer.size()) return false;
	const uint8_t *ptr=buffer.data()+offset;
	retValue=0;
	for (size_t i=0;i<len;i++)
		retValue=(retValue>>8)^CRCTable[(retValue&0xff)^ptr[i]];
	return true;
}

bool RNCDecompressor::detectHeader(uint32_t hdr)
{
	if (hdr==FourCC('RNC\001') || hdr==FourCC('RNC\002')) return true;
		return false;
}

RNCDecompressor::RNCDecompressor(const Buffer &packedData) :
	Decompressor(packedData),
	_isValid(false)
{
	uint32_t hdr;
	if (!packedData.read(0,hdr)) return;
	if (!packedData.read(4,_rawSize)) return;
	if (!packedData.read(8,_packedSize)) return;
	if (!_rawSize || !_packedSize) return;

	_ver=[&]()->RNCVersion
	{
		if (hdr==FourCC('RNC\001'))
		{
			// now detect between old and new version
			// since the old and the new version share the same id, there is no foolproof way
			// to tell them apart. It is easier to prove that it is not something by finding
			// specific invalid bitstream content.

			// if we can't access the old-stream we are nothing (since old stream is smaller the new stream by header 12 vs. 18)
			uint8_t oldStreamStart,newStreamStart;
			if (!packedData.read(_packedSize+11,oldStreamStart)) return RNCVersion::Invalid;

			// well, this is silly though but lets assume someone has made old format RNC1 with total size less than 19
			if (!packedData.read(18,newStreamStart)) return RNCVersion::RNC1Old;

			// Check that stream starts with a literal(s)
			if (!(oldStreamStart&0x80))
				return RNCVersion::RNC1New;

			// New stream have two bits in start as a filler on new stream. Those are always 0
			// (although this is not strictly mandated)
			// +
			// Even though it is possible to make new RNC1 stream which starts with zero literal table size,
			// it is extremely unlikely
			if ((newStreamStart&3) || !(newStreamStart&0x7c))
				return RNCVersion::RNC1Old;

			// now the last resort: check CRC.
			uint16_t hdrCrc,crc;
			if (!packedData.read(14,hdrCrc)) return RNCVersion::Invalid;
			if (RNCCRC(_packedData,18,_packedSize,crc) && crc==hdrCrc)
				return RNCVersion::RNC1New;
			else
				return RNCVersion::RNC1Old;

			// this would be good for files, where we know the size, but not for memory buffer.
			// it is not good idea to make separate logic for the 2
#if 0
			if (_packedSize+12==packedData.size())
			{
				return RNCVersion::RNC1Old;
			}
			return RNCVersion::RNC1New;
#endif
		} else if (hdr==FourCC('RNC\002')) {
			return RNCVersion::RNC2;
		} else {
			return RNCVersion::Invalid;
		}
	}();
	if (_ver==RNCVersion::Invalid) return;

	if (_ver!=RNCVersion::RNC1Old)
	{
		if (!packedData.read(12,_rawCRC)) return;
		if (!packedData.read(14,_packedCRC)) return;
		if (!packedData.read(17,_chunks)) return;
	}

	size_t hdrSize=(_ver==RNCVersion::RNC1Old)?12:18;
	// nope, messes with type detection & crc
#if 0
	// Hack: sometimes the packed size header is smaller than the actual data amount
	// since reading the bistream is non-destructive. let it go beyond the advertised size (but not beyond buffer)
	// works only on std. RNC1 & RNC2, not the old one.
	if (_ver!=RNCVersion::RNC1Old && _packedSize+hdrSize<packedData.size())
	{
		_packedSize=packedData.size()-hdrSize;
	}
#endif
	if (_packedSize+hdrSize<=packedData.size()) _isValid=true;
}

RNCDecompressor::~RNCDecompressor()
{
	// nothing needed
}

bool RNCDecompressor::isValid() const
{
	return _isValid;
}

bool RNCDecompressor::verifyPacked() const
{
	if (!_isValid) return false;
	if (_ver!=RNCVersion::RNC1Old)
	{
		uint16_t crc;
		return RNCCRC(_packedData,18,_packedSize,crc) && crc==_packedCRC;
	} else return true;
}

bool RNCDecompressor::verifyRaw(const Buffer &rawData) const
{
	if (!_isValid || rawData.size()<_rawSize) return false;
	if (_ver!=RNCVersion::RNC1Old)
	{
		uint16_t crc;
		return RNCCRC(rawData,0,_rawSize,crc) && crc==_rawCRC;
	} else return true;
}

size_t RNCDecompressor::getRawSize() const
{
	if (!_isValid) return 0;
	return _rawSize;
}

bool RNCDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_rawSize) return false;

	switch (_ver)
	{
		case RNCVersion::RNC1Old:
		return RNC1DecompressOld(rawData);

		case RNCVersion::RNC1New:
		return RNC1DecompressNew(rawData);

		case RNCVersion::RNC2:
		return RNC2Decompress(rawData);

		default:
		return false;
	}
}

bool RNCDecompressor::RNC1DecompressOld(Buffer &rawData)
{
	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_packedSize+12;

	// make sure the anchor-bit is not taken in as a data bit
	if (bufOffset==12) return false;
	uint8_t bufBitsContent=bufPtr[--bufOffset];
	uint8_t bufBitsLength=7;
	// the anchor-bit does not seem always to be at the correct place
	for (uint32_t i=0;i<7;i++)
		if (bufBitsContent&(1<<i)) break;
			else bufBitsLength--;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffset<=12)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=bufPtr[--bufOffset];
			bufBitsLength=8;
		}
		uint8_t ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<count;i++) ret=(ret<<1)|uint32_t(readBit());
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (!streamStatus || bufOffset<=12)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[--bufOffset];
	};

	HuffmanDecoder<uint8_t,0xffU,2> litDecoder
	{
		HuffmanCode<uint8_t>{1,0b00,0},
		HuffmanCode<uint8_t>{2,0b10,1},
		HuffmanCode<uint8_t>{2,0b11,2}
	};

	HuffmanDecoder<uint8_t,0xffU,4> lengthDecoder
	{
		HuffmanCode<uint8_t>{1,0b0000,0},
		HuffmanCode<uint8_t>{2,0b0010,1},
		HuffmanCode<uint8_t>{3,0b0110,2},
		HuffmanCode<uint8_t>{4,0b1110,3},
		HuffmanCode<uint8_t>{4,0b1111,4}
	};

	HuffmanDecoder<uint8_t,0xffU,2> distanceDecoder
	{
		HuffmanCode<uint8_t>{1,0b00,0},
		HuffmanCode<uint8_t>{2,0b10,1},
		HuffmanCode<uint8_t>{2,0b11,2}
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=_rawSize;

	while (streamStatus)
	{
		uint32_t litLength=litDecoder.decode(readBit);

		if (litLength==2)
		{
			static const uint32_t litBitLengths[4]={2,2,3,10};
			static const uint32_t litAdditions[4]={2,5,8,15};
			for (uint32_t i=0;i<4;i++)
			{
				litLength=readBits(litBitLengths[i]);
				if (litLength!=(1<<litBitLengths[i])-1 || i==3)
				{
					litLength+=litAdditions[i];
					break;
				}
			}
		}
		
		if (!streamStatus || destOffset<litLength)
		{
			streamStatus=false;
			break;
		} else {
			for (uint32_t i=0;i<litLength;i++) dest[--destOffset]=readByte();
		}
	
		// the only way to successfully end the loop!
		if (!destOffset) break;

		uint32_t count;
		{
			uint32_t lengthIndex=lengthDecoder.decode(readBit);
			static const uint32_t lengthBitLengths[5]={0,0,1,2,10};
			static const uint32_t lengthAdditions[5]={2,3,4,6,10};
			count=readBits(lengthBitLengths[lengthIndex])+lengthAdditions[lengthIndex];
		}

		uint32_t distance;
		if (count!=2)
		{
			uint32_t distanceIndex=distanceDecoder.decode(readBit);
			static const uint32_t distanceBitLengths[3]={8,5,12};
			static const uint32_t distanceAdditions[3]={32,0,288};
			distance=readBits(distanceBitLengths[distanceIndex])+distanceAdditions[distanceIndex];
		} else {
			if (!readBit())
			{
				distance=readBits(6);
			} else {
				distance=readBits(9)+64;
			}
		}

		uint32_t distOffset=(distance)?distance+count-1:1;
		if (destOffset<count || destOffset+distOffset>_rawSize)
		{
			streamStatus=false;
		} else {
			distOffset+=destOffset;
			for (uint32_t i=0;i<count;i++) dest[--destOffset]=dest[--distOffset];
		}
	}
	return streamStatus && !destOffset;
}

bool RNCDecompressor::RNC1DecompressNew(Buffer &rawData)
{
	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data()+18;
	size_t bufOffset=0;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint8_t bits)->uint32_t
	{
		uint32_t ret=0;
		uint8_t retBits=0;
		while (streamStatus && retBits!=bits)
		{
			if (!bufBitsLength)
			{
				if (bufOffset>=_packedSize)
				{
					streamStatus=false;
					return 0;
				}
				bufBitsContent=bufPtr[bufOffset++];
				bufBitsLength=8;
				if (bufOffset<_packedSize)
				{
					bufBitsContent|=bufPtr[bufOffset++]<<8;
					bufBitsLength+=8;
				}
			}
			uint8_t bitsToTake=std::min<uint8_t>(bits-retBits,bufBitsLength);
			ret|=(bufBitsContent&((1<<bitsToTake)-1))<<retBits;
			retBits+=bitsToTake;
			bufBitsContent>>=bitsToTake;
			bufBitsLength-=bitsToTake;
		}
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (!streamStatus || bufOffset>=_packedSize)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[bufOffset++];
	};


	typedef HuffmanDecoder<int32_t,-1,0> RNC1HuffmanDecoder;

	// helpers
	auto readHuffmanTable=[&](RNC1HuffmanDecoder &dec)
	{
		uint32_t length=readBits(5);
		// not much to decode from here...
		if (!length) return;
		uint32_t maxDepth=0;
		uint32_t lengthTable[length];
		for (uint32_t i=0;i<length&&streamStatus;i++)
		{
			lengthTable[i]=readBits(4);
			if (lengthTable[i]>maxDepth) maxDepth=lengthTable[i];
		}

		uint32_t code=0;
		for (uint32_t depth=1;depth<=maxDepth;depth++)
		{
			for (uint32_t i=0;i<length;i++)
			{
				if (depth==lengthTable[i])
				{
					dec.insert(HuffmanCode<int32_t>{depth,code>>(maxDepth-depth),int32_t(i)});
					code+=1<<(maxDepth-depth);
				}
			}
		}
		if (streamStatus) streamStatus=dec.isValid();
	};

	auto huffmanDecode=[&](const RNC1HuffmanDecoder &dec)->int32_t
	{
		// this is kind of non-specced
		int32_t ret=dec.decode([&]()->uint32_t{return readBits(1);});
		if (ret>=2)
			ret=(1<<(ret-1))|readBits(ret-1);
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	auto processLiterals=[&](const RNC1HuffmanDecoder &dec)->void
	{
		if (!streamStatus) return;
		int32_t litLength=huffmanDecode(dec);
		if (litLength<0 || destOffset+litLength>_rawSize)
		{
			streamStatus=false;
		} else {
			for (int32_t i=0;i<litLength;i++) dest[destOffset++]=readByte();
		}
	};

	readBits(2);
	for (uint8_t chunks=0;chunks<_chunks&&streamStatus;chunks++)
	{
		RNC1HuffmanDecoder litDecoder,distanceDecoder,lengthDecoder;
		readHuffmanTable(litDecoder);
		readHuffmanTable(distanceDecoder);
		readHuffmanTable(lengthDecoder);
		uint32_t count=readBits(16);

		for (uint32_t sub=1;sub<count&&streamStatus;sub++)
		{
			processLiterals(litDecoder);
			int32_t distance=huffmanDecode(distanceDecoder);
			int32_t count=huffmanDecode(lengthDecoder);
			if (!streamStatus || distance==-1 || count==-1 || size_t(distance+1)>destOffset || destOffset+count+2>_rawSize)
			{
				streamStatus=false;
			} else {
				distance++;
				count+=2;
				for (int32_t i=0;i<count;i++,destOffset++)
					dest[destOffset]=dest[destOffset-distance];
			}
		}
		processLiterals(litDecoder);
	}

	return streamStatus && _rawSize==destOffset;
}

bool RNCDecompressor::RNC2Decompress(Buffer &rawData)
{
	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data()+18;
	size_t bufOffset=0;
	uint8_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffset>=_packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=bufPtr[bufOffset++];
			bufBitsLength=8;
		}
		uint8_t ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (!streamStatus || bufOffset>=_packedSize)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[bufOffset++];
	};

	// Huffman decoding
	enum class Cmd
	{
		INV,	// Invalid
		LIT,	// 0, Literal
		MOV,	// 10, Move bytes + length + distance, Get bytes if length=9 + 4bits
		MV2,	// 110, Move 2 bytes
		MV3,	// 1110, Move 3 bytes
		CND	// 1111, Conditional copy, or EOF
		
	};

	HuffmanDecoder<Cmd,Cmd::INV,4> cmdDecoder
	{
		HuffmanCode<Cmd>{1,0b0000,Cmd::LIT},
		HuffmanCode<Cmd>{2,0b0010,Cmd::MOV},
		HuffmanCode<Cmd>{3,0b0110,Cmd::MV2},
		HuffmanCode<Cmd>{4,0b1110,Cmd::MV3},
		HuffmanCode<Cmd>{4,0b1111,Cmd::CND}
	};

	/* length of 9 is a marker for literals */
	HuffmanDecoder<uint8_t,0,3> lengthDecoder
	{
		HuffmanCode<uint8_t>{2,0b000,4},
		HuffmanCode<uint8_t>{2,0b010,5},
		HuffmanCode<uint8_t>{3,0b010,6},
		HuffmanCode<uint8_t>{3,0b011,7},
		HuffmanCode<uint8_t>{3,0b110,8},
		HuffmanCode<uint8_t>{3,0b111,9}
	};
	
	HuffmanDecoder<int8_t,-1,6> distanceDecoder
	{
		HuffmanCode<int8_t>{1,0b000000,0},
		HuffmanCode<int8_t>{3,0b000110,1},
		HuffmanCode<int8_t>{4,0b001000,2},
		HuffmanCode<int8_t>{4,0b001001,3},
		HuffmanCode<int8_t>{5,0b010101,4},
		HuffmanCode<int8_t>{5,0b010111,5},
		HuffmanCode<int8_t>{5,0b011101,6},
		HuffmanCode<int8_t>{5,0b011111,7},
		HuffmanCode<int8_t>{6,0b101000,8},
		HuffmanCode<int8_t>{6,0b101001,9},
		HuffmanCode<int8_t>{6,0b101100,10},
		HuffmanCode<int8_t>{6,0b101101,11},
		HuffmanCode<int8_t>{6,0b111000,12},
		HuffmanCode<int8_t>{6,0b111001,13},
		HuffmanCode<int8_t>{6,0b111100,14},
		HuffmanCode<int8_t>{6,0b111101,15}
	};


	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	// helpers
	auto readDistance=[&]()->uint32_t
	{
		int8_t distMult=distanceDecoder.decode(readBit);
		if (distMult<0) streamStatus=false;
		uint8_t distByte=readByte();
		return (uint32_t(distByte)|(uint32_t(distMult)<<8))+1;
	};
	
	auto moveBytes=[&](uint32_t distance,uint32_t count)->void
	{
		if (!count || distance>destOffset || destOffset+count>_rawSize)
		{
			streamStatus=false;
		} else {
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	};

	readBit();
	readBit();
	uint8_t foundChunks=0;
	bool done=false;
	while (streamStatus && !done && foundChunks<_chunks)
	{
		Cmd cmd=cmdDecoder.decode(readBit);
		switch (cmd) {
			case Cmd::INV:
			streamStatus=false;
			break;

			case Cmd::LIT:
			if (destOffset<_rawSize) dest[destOffset++]=readByte();
				else streamStatus=false;
			break;

			case Cmd::MOV:
			{
				uint8_t count=lengthDecoder.decode(readBit);
				if (count!=9)
					moveBytes(readDistance(),count);
				else {
					uint32_t rep=0;
					for (uint32_t i=0;i<4;i++)
						rep=(rep<<1)|readBit();
					rep=(rep+3)*4;
					if (destOffset+rep>_rawSize) streamStatus=false;
					if (streamStatus)
						for (uint32_t i=0;i<rep;i++,destOffset++)
							dest[destOffset]=readByte();
				}
			}
			break;

			case Cmd::MV2:
			moveBytes(uint32_t(readByte())+1,2);
			break;

			case Cmd::MV3:
			moveBytes(readDistance(),3);
			break;

			case Cmd::CND:
			{
				uint8_t count=readByte();
				if (count)
					moveBytes(readDistance(),uint32_t(count+8));
				else {
					foundChunks++;
					done=!readBit();
				}
				
			}			
			break;
		}
	}

	return streamStatus && _rawSize==destOffset && _chunks==foundChunks;
}
