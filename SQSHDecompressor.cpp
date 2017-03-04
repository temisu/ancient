/* Copyright (C) Teemu Suutari */

#include <string.h>

#include "SQSHDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool SQSHDecompressor::detectHeader(uint32_t hdr)
{
	if (hdr==FourCC('SQSH')) return true;
		else return false;
}

SQSHDecompressor::SQSHDecompressor(uint32_t hdr,const Buffer &packedData) :
	Decompressor(packedData)
{
	if (!detectHeader(hdr)) return;
	uint16_t tmp;
	if (!packedData.read(0,tmp)) return;
	_rawSize=uint32_t(tmp);

	_isValid=true;
}

SQSHDecompressor::~SQSHDecompressor()
{
}

bool SQSHDecompressor::isValid() const
{
	return _isValid;
}

bool SQSHDecompressor::verifyPacked() const
{
	return _isValid;
}

bool SQSHDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

size_t SQSHDecompressor::getRawSize() const
{
	if (!_isValid) return 0;
	return _rawSize;
}

bool SQSHDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid || _packedData.size()<3 || !_rawSize || rawData.size()<_rawSize) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=2;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint8_t bits)->uint32_t
	{
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=(bufBitsContent<<8)|bufPtr[bufOffset++];
			bufBitsLength+=8;
		}

		uint32_t ret=(bufBitsContent>>(bufBitsLength-bits))&((1<<bits)-1);
		bufBitsLength-=bits;
		return ret;
	};
	
	auto readBit=[&]()->uint32_t
	{
		return readBits(1);
	};

	auto readSignedBits=[&](uint8_t bits)->int32_t
	{
		int32_t ret=readBits(bits);
		if (ret&(1<<(bits-1)))
			ret|=~0U<<bits;
		return ret;
	};

	HuffmanDecoder<uint8_t,0xffU,4> modDecoder
	{
		HuffmanCode<uint8_t>{1,0b0001,0},
		HuffmanCode<uint8_t>{2,0b0000,1},
		HuffmanCode<uint8_t>{3,0b0010,2},
		HuffmanCode<uint8_t>{4,0b0110,3},
		HuffmanCode<uint8_t>{4,0b0111,4}
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
		HuffmanCode<uint8_t>{1,0b01,0},
		HuffmanCode<uint8_t>{2,0b00,1},
		HuffmanCode<uint8_t>{2,0b01,2}
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	// first byte is special
	uint8_t currentSample=bufPtr[bufOffset++];
	dest[destOffset++]=currentSample;

	uint32_t accum1=0,accum2=0,prevBits=0;

	while (streamStatus && destOffset!=_rawSize)
	{
		uint8_t bits;
		uint32_t count;
		bool doRepeat=false;

		if (accum1>=8)
		{
			static const uint8_t bitLengthTable[7][8]={
				{2,3,4,5,6,7,8,0},
				{3,2,4,5,6,7,8,0},
				{4,3,5,2,6,7,8,0},
				{5,4,6,2,3,7,8,0},
				{6,5,7,2,3,4,8,0},
				{7,6,8,2,3,4,5,0},
				{8,7,6,2,3,4,5,0}};

			auto handleCondCase=[&]()
			{
				if (bits==8) {
					if (accum2<20)
					{
						count=1;
					} else {
						count=2;
						accum2+=8;
					}
				} else {
					count=5;
					accum2+=8;
				}
			};

			auto handleTable=[&](uint32_t newBits)
			{
				if (prevBits<2 || !newBits)
				{
					streamStatus=false;
					return;
				}
				bits=bitLengthTable[prevBits-2][newBits-1];
				if (!bits)
				{
					streamStatus=false;
					return;
				}
				handleCondCase();
			};

			uint32_t mod=modDecoder.decode(readBit);
			switch (mod)
			{
				case 0:
				if (prevBits==8)
				{
					bits=8;
					handleCondCase();
				} else {
					bits=prevBits;
					count=5;
					accum2+=8;
				}
				break;

				case 1:
				doRepeat=true;
				break;

				case 2:
				handleTable(2);
				break;

				case 3:
				handleTable(3);
				break;

				case 4:
				handleTable(readBits(2)+4);
				break;

				default:
				streamStatus=false;
				break;
			}
		} else {
			if (readBit())
			{
				doRepeat=true;
			} else {
				count=1;
				bits=8;
			}
		}

		if (doRepeat) {
			uint32_t lengthIndex=lengthDecoder.decode(readBit);
			static const uint8_t lengthBits[5]={1,1,1,3,5};
			static const uint32_t lengthAdditions[5]={2,4,6,8,16};
			count=readBits(lengthBits[lengthIndex])+lengthAdditions[lengthIndex];
			if (count>=3)
			{
				if (accum1) accum1--;
				if (count>3 && accum1) accum1--;
			}
			uint32_t distanceIndex=distanceDecoder.decode(readBit);
			static const uint8_t distanceBits[3]={12,8,14};
			static const uint32_t distanceAdditions[3]={0x101,1,0x1101};
			uint32_t distance=readBits(distanceBits[distanceIndex])+distanceAdditions[distanceIndex];
			if (destOffset+count>_rawSize)
				count=uint32_t(_rawSize-destOffset);
			if (!streamStatus || distance>destOffset)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count;i++,destOffset++)
					dest[destOffset]=dest[destOffset-distance];
			}
			currentSample=dest[destOffset-1];
		} else {
			if (destOffset+count>_rawSize)
				count=uint32_t(_rawSize-destOffset);
			if (streamStatus)
			{
				for (uint32_t i=0;i<count;i++)
				{
					currentSample-=readSignedBits(bits);
					dest[destOffset++]=currentSample;
				}
			}
			if (accum1!=31) accum1++;
			prevBits=bits;
		}

		accum2-=accum2>>3;
	}

	return streamStatus && destOffset==_rawSize;
}
