/* Copyright (C) Teemu Suutari */

#include <algorithm>

#include "RNCDecompressor.hpp"
#include "HuffmanDecoder.hpp"

#include <CRC16.hpp>

bool RNCDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC('RNC\001') || hdr==FourCC('RNC\002');
}

std::unique_ptr<Decompressor> RNCDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<RNCDecompressor>(packedData,verify);
}

RNCDecompressor::RNCDecompressor(const Buffer &packedData,bool verify) :
	_packedData(packedData)
{
	uint32_t hdr=packedData.readBE32(0);
	_rawSize=packedData.readBE32(4);
	_packedSize=packedData.readBE32(8);
	if (!_rawSize || !_packedSize ||
		_rawSize>getMaxRawSize() || _packedSize>getMaxPackedSize()) throw InvalidFormatError();

	bool verified=false;
	if (hdr==FourCC('RNC\001'))
	{
		// now detect between old and new version
		// since the old and the new version share the same id, there is no foolproof way
		// to tell them apart. It is easier to prove that it is not something by finding
		// specific invalid bitstream content.

		// well, this is silly though but lets assume someone has made old format RNC1 with total size less than 19
		if (packedData.size()<19)
		{
			_ver=Version::RNC1Old;
		} else {
			uint8_t newStreamStart=packedData.read8(18);
			uint8_t oldStreamStart=packedData.read8(_packedSize+11);

			// Check that stream starts with a literal(s)
			if (!(oldStreamStart&0x80))
				_ver=Version::RNC1New;

			// New stream have two bits in start as a filler on new stream. Those are always 0
			// (although this is not strictly mandated)
			// +
			// Even though it is possible to make new RNC1 stream which starts with zero literal table size,
			// it is extremely unlikely
			else if ((newStreamStart&3) || !(newStreamStart&0x7c))
				_ver=Version::RNC1Old;

			// now the last resort: check CRC.
			else if (_packedData.size()>=_packedSize+18 && CRC16(_packedData,18,_packedSize,0)==packedData.readBE16(14))
			{
				_ver=Version::RNC1New;
				verified=true;
			} else _ver=Version::RNC1Old;
		}
	} else if (hdr==FourCC('RNC\002')) {
		_ver=Version::RNC2;
	} else throw InvalidFormatError();

	size_t hdrSize=(_ver==Version::RNC1Old)?12:18;
	if (_packedSize+hdrSize>packedData.size()) throw InvalidFormatError();

	if (_ver!=Version::RNC1Old)
	{
		_rawCRC=packedData.readBE16(12);
		_chunks=packedData.read8(17);
		if (verify && !verified)
		{
			if (CRC16(_packedData,18,_packedSize,0)!=packedData.readBE16(14))
				throw VerificationError();
		}
	}
}

RNCDecompressor::~RNCDecompressor()
{
	// nothing needed
}

const std::string &RNCDecompressor::getName() const noexcept
{
	static std::string names[3]={
		"RNC1: Rob Northen RNC1 Compressor (old)",
		"RNC1: Rob Northen RNC1 Compressor ",
		"RNC2: Rob Northen RNC2 Compressor"};
	return names[static_cast<uint32_t>(_ver)];
}

size_t RNCDecompressor::getPackedSize() const noexcept
{
	if (_ver==Version::RNC1Old) return _packedSize+12;
		else return _packedSize+18;
}

size_t RNCDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void RNCDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	switch (_ver)
	{
		case Version::RNC1Old:
		return RNC1DecompressOld(rawData,verify);

		case Version::RNC1New:
		return RNC1DecompressNew(rawData,verify);

		case Version::RNC2:
		return RNC2Decompress(rawData,verify);

		default:
		throw DecompressionError();
	}
}

void RNCDecompressor::RNC1DecompressOld(Buffer &rawData,bool verify)
{
	// Stream reading
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_packedSize+12;

	// make sure the anchor-bit is not taken in as a data bit
	if (bufOffset==12) throw DecompressionError();
	uint8_t bufBitsContent=bufPtr[--bufOffset];
	uint8_t bufBitsLength=7;
	// the anchor-bit does not seem always to be at the correct place
	for (uint32_t i=0;i<7;i++)
		if (bufBitsContent&(1<<i)) break;
			else bufBitsLength--;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset<=12) throw DecompressionError();
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
		if (bufOffset<=12) throw DecompressionError();
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

	for (;;)
	{
		uint32_t litLength=litDecoder.decode(readBit);

		if (litLength==2)
		{
			static const uint32_t litBitLengths[4]={2,2,3,10};
			static const uint32_t litAdditions[4]={2,5,8,15};
			for (uint32_t i=0;i<4;i++)
			{
				litLength=readBits(litBitLengths[i]);
				if (litLength!=(1U<<litBitLengths[i])-1U || i==3)
				{
					litLength+=litAdditions[i];
					break;
				}
			}
		}
		
		if (destOffset<litLength) throw DecompressionError();
		for (uint32_t i=0;i<litLength;i++) dest[--destOffset]=readByte();
	
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
		if (destOffset<count || destOffset+distOffset>_rawSize) throw DecompressionError();
		distOffset+=destOffset;
		for (uint32_t i=0;i<count;i++) dest[--destOffset]=dest[--distOffset];
	}
}

void RNCDecompressor::RNC1DecompressNew(Buffer &rawData,bool verify)
{
	// Stream reading
	const uint8_t *bufPtr=_packedData.data()+18;
	size_t bufOffset=0;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint8_t bits)->uint32_t
	{
		uint32_t ret=0;
		uint8_t retBits=0;
		while (retBits!=bits)
		{
			if (!bufBitsLength)
			{
				if (bufOffset>=_packedSize) throw DecompressionError();
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
		if (bufOffset>=_packedSize) throw DecompressionError();
		return bufPtr[bufOffset++];
	};


	typedef HuffmanDecoder<uint32_t,0x100U,0> RNC1HuffmanDecoder;

	// helpers
	auto readHuffmanTable=[&](RNC1HuffmanDecoder &dec)
	{
		uint32_t length=readBits(5);
		// not much to decode from here...
		if (!length) return;
		uint32_t maxDepth=0;
		uint8_t lengthTable[length];
		for (uint32_t i=0;i<length;i++)
		{
			lengthTable[i]=readBits(4);
			if (lengthTable[i]>maxDepth) maxDepth=lengthTable[i];
		}

		CreateOrderlyHuffmanTable(dec,lengthTable,length);
	};

	auto huffmanDecode=[&](const RNC1HuffmanDecoder &dec)->int32_t
	{
		// this is kind of non-specced
		uint32_t ret=dec.decode([&]()->uint32_t{return readBits(1);});
		if (ret>=2)
			ret=(1<<(ret-1))|readBits(ret-1);
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	auto processLiterals=[&](const RNC1HuffmanDecoder &dec)
	{
		uint32_t litLength=huffmanDecode(dec);
		if (destOffset+litLength>_rawSize) throw DecompressionError();
		for (uint32_t i=0;i<litLength;i++) dest[destOffset++]=readByte();
	};

	readBits(2);
	for (uint8_t chunks=0;chunks<_chunks;chunks++)
	{
		RNC1HuffmanDecoder litDecoder,distanceDecoder,lengthDecoder;
		readHuffmanTable(litDecoder);
		readHuffmanTable(distanceDecoder);
		readHuffmanTable(lengthDecoder);
		uint32_t count=readBits(16);

		for (uint32_t sub=1;sub<count;sub++)
		{
			processLiterals(litDecoder);
			uint32_t distance=huffmanDecode(distanceDecoder);
			uint32_t count=huffmanDecode(lengthDecoder);
			if (size_t(distance+1)>destOffset || destOffset+count+2>_rawSize) throw DecompressionError();
			distance++;
			count+=2;
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
		processLiterals(litDecoder);
	}

	if (_rawSize!=destOffset) throw DecompressionError();
	if (verify && CRC16(rawData,0,_rawSize,0)!=_rawCRC) throw VerificationError();
}

void RNCDecompressor::RNC2Decompress(Buffer &rawData,bool verify)
{
	// Stream reading
	const uint8_t *bufPtr=_packedData.data()+18;
	size_t bufOffset=0;
	uint8_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset>=_packedSize) throw DecompressionError();
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
		if (bufOffset>=_packedSize) throw DecompressionError();
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
		if (distMult<0) throw DecompressionError();
		uint8_t distByte=readByte();
		return (uint32_t(distByte)|(uint32_t(distMult)<<8))+1;
	};
	
	auto moveBytes=[&](uint32_t distance,uint32_t count)->void
	{
		if (!count || distance>destOffset || destOffset+count>_rawSize) throw DecompressionError();
		for (uint32_t i=0;i<count;i++,destOffset++)
			dest[destOffset]=dest[destOffset-distance];
	};

	readBit();
	readBit();
	uint8_t foundChunks=0;
	bool done=false;
	while (!done && foundChunks<_chunks)
	{
		Cmd cmd=cmdDecoder.decode(readBit);
		switch (cmd) {
			case Cmd::INV:
			throw DecompressionError();
			break;

			case Cmd::LIT:
			if (destOffset>=_rawSize) throw DecompressionError();
			dest[destOffset++]=readByte();
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
					if (destOffset+rep>_rawSize) throw DecompressionError();
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

	if (_rawSize!=destOffset || _chunks!=foundChunks) throw DecompressionError();
	if (verify && CRC16(rawData,0,_rawSize,0)!=_rawCRC) throw VerificationError();
}

Decompressor::Registry<RNCDecompressor> RNCDecompressor::_registration;
