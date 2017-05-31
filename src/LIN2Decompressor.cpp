/* Copyright (C) Teemu Suutari */

#include "LIN2Decompressor.hpp"
#include "HuffmanDecoder.hpp"

bool LIN2Decompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('LIN2') || hdr==FourCC('LIN4');
}

bool LIN2Decompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> LIN2Decompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<LIN2Decompressor>(hdr,packedData,state);
}

LIN2Decompressor::LIN2Decompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_ver=(hdr==FourCC('LIN2'))?2:4;
	if (packedData.size()<10) return;

	uint32_t tmp;
	if (!packedData.readBE(0,tmp) || tmp) return;	// password set

	// LIN4 is very similar to LIN2 - it only has 5 bit literals instead of 4 bit literals
	// (and thus larger table at the end of the stream)
	// Also, the huffman decoder for length is different

	_endStreamOffset=packedData.size()-1;
	const uint8_t *bufPtr=_packedData.data();
	while (_endStreamOffset && bufPtr[--_endStreamOffset]!=0xffU);
	// end stream
	// 1 byte, byte before 0xff
	// 0x10 bytes/0x20 for table
	if (_endStreamOffset<0x11+0xa) return;
	_endStreamOffset-=(_ver==2)?0x11:0x21;

	size_t midStreamOffset=(_ver==2)?0x16:0x26;
	// midstream
	// from endstream without
	// add 0x10/0x20 byte back to point after table
	// add 6 bytes to point to correct place

	if (!packedData.readBE(4,tmp)) return;
	if (_endStreamOffset+midStreamOffset<tmp+10 || tmp<midStreamOffset) return;
	_midStreamOffset=_endStreamOffset-tmp+midStreamOffset;

	_isValid=true;
}

LIN2Decompressor::~LIN2Decompressor()
{
	// nothing needed
}

bool LIN2Decompressor::isValid() const
{
	return _isValid;
}

bool LIN2Decompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool LIN2Decompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &LIN2Decompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name2="XPK-LIN2: LIN2 LINO packer";
	static std::string name4="XPK-LIN4: LIN4 LINO packer";
	return (_ver==2)?name2:name4;
}

bool LIN2Decompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t bufBitsOffset=10;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	// three streams.
	// 1. ordinary bit stream out of words (readBits)
	// 2. bit stream for literals (readBit)
	// 3. nibble stream for literal (read4Bits)
	// at the end of the stream there is a literal table of 16 bytes
	// apart from confusing naming, there are also some nasty
	// interdependencies :(

	auto readBits=[&](uint8_t bits)->uint32_t
	{
		if (!streamStatus) return 0;
		while (bufBitsLength<bits)
		{
			if (bufBitsOffset>=_midStreamOffset)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent<<=8;
			bufBitsContent|=uint32_t(bufPtr[bufBitsOffset++]);
			bufBitsLength+=8;
		}
		uint32_t ret=(bufBitsContent>>(bufBitsLength-bits))&((1<<bits)-1);
		bufBitsLength-=bits;
		return ret;
	};

	size_t bufBitOffset=_midStreamOffset;
	size_t buf4BitsOffset=_endStreamOffset;
	bool buf4Incomplete=false;
	{
		uint8_t tmp;
		if (!_packedData.read(9,tmp)) return false;
		buf4Incomplete=!!tmp;
		if (buf4Incomplete)
		{
			if (buf4BitsOffset<=bufBitOffset) return false;
			buf4BitsOffset--;
		}
	}

	uint8_t bufBitContent=0;
	uint8_t bufBitLength=0;
	if (bufBitOffset+1>=buf4BitsOffset)
	{
		return false;
	} else {
		bufBitLength=8-bufPtr[bufBitOffset++];
		if (bufBitLength>8) return false;
		bufBitContent=bufPtr[bufBitOffset++];
	}

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitLength)
		{
			if (!streamStatus || bufBitOffset>=buf4BitsOffset)
			{
				streamStatus=false;
				return 0;
			}
			bufBitContent=bufPtr[bufBitOffset++];
			bufBitLength=8;
		}
		uint8_t ret=bufBitContent>>7;
		bufBitContent<<=1;
		bufBitLength--;
		return ret;
	};

	// this is a rather strange thing...
	auto read4Bits=[&](uint32_t multiple)->uint8_t
	{
		if (multiple==1)
		{
			if (buf4Incomplete)
			{
				buf4Incomplete=false;
				return bufPtr[buf4BitsOffset]&0xf;
			} else {
				if (!streamStatus || buf4BitsOffset<=bufBitOffset)
				{
					streamStatus=false;
					return 0;
				}
				buf4Incomplete=true;
				return bufPtr[--buf4BitsOffset]>>4;
			}
		} else {
			// a byte
			if (!streamStatus || buf4BitsOffset<=bufBitOffset)
			{
				streamStatus=false;
				return 0;
			}
			if (buf4Incomplete)
			{
				uint8_t ret=bufPtr[buf4BitsOffset]&0xf;
				ret|=bufPtr[--buf4BitsOffset]&0xf0U;
				return ret;
			} else {
				return bufPtr[--buf4BitsOffset];
			}
		}
	};

	// little meh to initialize both (intentionally deleted copy/assign)
	HuffmanDecoder<uint8_t,0xffU,7> lengthDecoder2
	{
		HuffmanCode<uint8_t>{1,0b000000,3},
		HuffmanCode<uint8_t>{3,0b000100,4},
		HuffmanCode<uint8_t>{3,0b000101,5},
		HuffmanCode<uint8_t>{3,0b000110,6},
		HuffmanCode<uint8_t>{6,0b111000,7},
		HuffmanCode<uint8_t>{6,0b111001,8},
		HuffmanCode<uint8_t>{6,0b111010,9},
		HuffmanCode<uint8_t>{6,0b111011,10},
		HuffmanCode<uint8_t>{6,0b111100,11},
		HuffmanCode<uint8_t>{6,0b111101,12},
		HuffmanCode<uint8_t>{6,0b111110,13},
		HuffmanCode<uint8_t>{6,0b111111,0}
	};

	HuffmanDecoder<uint8_t,0xffU,7> lengthDecoder4
	{
		HuffmanCode<uint8_t>{2,0b0000000,3},
		HuffmanCode<uint8_t>{2,0b0000001,4},
		HuffmanCode<uint8_t>{2,0b0000010,5},
		HuffmanCode<uint8_t>{4,0b0001100,6},
		HuffmanCode<uint8_t>{4,0b0001101,7},
		HuffmanCode<uint8_t>{4,0b0001110,8},
		HuffmanCode<uint8_t>{7,0b1111000,9},
		HuffmanCode<uint8_t>{7,0b1111001,10},
		HuffmanCode<uint8_t>{7,0b1111010,11},
		HuffmanCode<uint8_t>{7,0b1111011,12},
		HuffmanCode<uint8_t>{7,0b1111100,13},
		HuffmanCode<uint8_t>{7,0b1111101,14},
		HuffmanCode<uint8_t>{7,0b1111110,15},
		HuffmanCode<uint8_t>{7,0b1111111,0}
	};
	auto &lengthDecoder=(_ver==2)?lengthDecoder2:lengthDecoder4;

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	uint32_t minBits=1;

	while (streamStatus && destOffset!=rawSize)
	{
		if (!readBits(1))
		{
			if (readBit())
			{
				dest[destOffset++]=read4Bits(2);
			} else {
				if (_ver==4)
				{
					dest[destOffset++]=bufPtr[_endStreamOffset+(read4Bits(1)<<1)+readBit()];
				} else dest[destOffset++]=bufPtr[_endStreamOffset+read4Bits(1)];
			}
		} else {
			uint32_t count=lengthDecoder.decode([&](){return readBits(1);});
			if (!count)
			{
				count=readBits(4);
				if (count==0xfU)
				{
					count=readBits(8);
					if (count==0xffU) break;
						else count+=3;
				} else count+=(_ver==2)?14:16;
			}

			uint32_t distance;
			bool isMax=false;
			do {
				uint32_t bits=readBits(3)+minBits;
				distance=readBits(bits);
				isMax=(distance==((1<<bits)-1))&&(bits==minBits+7);
				if (isMax) minBits++;
				distance+=(((1<<bits)-1)&~((1<<minBits)-1))+1;
			} while (isMax);

			// buggy compressors
			if (destOffset+count>rawSize) count=uint32_t(rawSize-destOffset);
			if (!count) break;

			if (!streamStatus || distance>destOffset || destOffset+count>rawSize)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count;i++,destOffset++)
					dest[destOffset]=dest[destOffset-distance];
			}
		}
	}

	return streamStatus && destOffset==rawSize;
}
