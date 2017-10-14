/* Copyright (C) Teemu Suutari */

#include "RDCNDecompressor.hpp"

bool RDCNDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('RDCN');
}

std::unique_ptr<XPKDecompressor> RDCNDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<RDCNDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

RDCNDecompressor::RDCNDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

RDCNDecompressor::~RDCNDecompressor()
{
	// nothing needed
}

const std::string &RDCNDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-RDCN: Ross data compression";
	return name;
}

void RDCNDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
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
				count+=3;
				doRLE=true;
				break;

				case 1:
				count=(count|(uint32_t(readByte())<<4))+19;
				repeatChar=readByte();
				doRLE=true;
				break;

				case 2:
				distance=(count|(uint32_t(readByte())<<4))+3;
				count=uint32_t(readByte())+16;
				break;

				default: /* 3 to 15 */
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

XPKDecompressor::Registry<RDCNDecompressor> RDCNDecompressor::_XPKregistration;
