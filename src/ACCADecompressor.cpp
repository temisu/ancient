/* Copyright (C) Teemu Suutari */

#include "ACCADecompressor.hpp"

bool ACCADecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('ACCA');
}

std::unique_ptr<XPKDecompressor> ACCADecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<ACCADecompressor>(hdr,recursionLevel,packedData,state,verify);
}

ACCADecompressor::ACCADecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

ACCADecompressor::~ACCADecompressor()
{
	// nothing needed
}

const std::string &ACCADecompressor::getSubName() const noexcept
{
	static std::string name="XPK-ACCA: Andre's code compression algorithm";
	return name;
}

void ACCADecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	uint16_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset+1>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=uint16_t(bufPtr[bufOffset++])<<8;
			bufBitsContent|=uint16_t(bufPtr[bufOffset++]);
			bufBitsLength=16;
		}
		uint8_t ret=bufBitsContent>>15;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (destOffset!=rawSize)
	{
		if (!readBit())
		{
			dest[destOffset++]=readByte();
		} else {
			static const uint8_t staticBytes[16]={
				0x00,0x01,0x02,0x03,0x04,0x08,0x10,0x20,
				0x40,0x55,0x60,0x80,0xaa,0xc0,0xe0,0xff};

			uint8_t tmp=readByte();
			uint32_t count=tmp&0xf;
			uint32_t code=tmp>>4;
			uint32_t distance=0;
			uint8_t repeatChar=0;
			bool doRLE=false;
			switch (code)
			{
				case 0:
				repeatChar=readByte();
				case 14:
				count+=3;
				doRLE=true;
				break;

				case 1:
				count=(count|(uint32_t(readByte())<<4))+19;
				repeatChar=readByte();
				doRLE=true;
				break;

				case 2:
				repeatChar=staticBytes[count];
				count=2;
				doRLE=true;
				break;

				case 15:
				distance=(count|(uint32_t(readByte())<<4))+3;
				count=uint32_t(readByte())+14;
				break;

				default: /* 3 to 13 */
				distance=(count|(uint32_t(readByte())<<4))+3;
				count=code;
				break;
			}
			if (doRLE)
			{
				if (destOffset+count>rawSize) throw Decompressor::DecompressionError();
				for (uint32_t i=0;i<count;i++) dest[destOffset++]=repeatChar;
			} else {
				if (distance>destOffset || destOffset+count>rawSize) throw Decompressor::DecompressionError();
				for (uint32_t i=0;i<count;i++,destOffset++)
					dest[destOffset]=dest[destOffset-distance];
			}
		}
	}
}

XPKDecompressor::Registry<ACCADecompressor> ACCADecompressor::_XPKregistration;
