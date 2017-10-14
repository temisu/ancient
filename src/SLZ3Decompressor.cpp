/* Copyright (C) Teemu Suutari */

#include "SLZ3Decompressor.hpp"

bool SLZ3Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('SLZ3');
}

std::unique_ptr<XPKDecompressor> SLZ3Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<SLZ3Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

SLZ3Decompressor::SLZ3Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

SLZ3Decompressor::~SLZ3Decompressor()
{
	// nothing needed
}

const std::string &SLZ3Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-SLZ3: SLZ3 CyberYAFA compressor";
	return name;
}

void SLZ3Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	uint8_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=uint16_t(bufPtr[bufOffset++]);
			bufBitsLength=8;
		}
		uint8_t ret=bufBitsContent>>7;
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
			if (!tmp) break;
			uint32_t distance=uint32_t(tmp&0xf0)<<4;
			distance|=uint32_t(readByte());
			uint32_t count=uint32_t(tmp&0xf)+2;
			if (!distance || distance>destOffset || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}

	if (destOffset!=rawSize) throw Decompressor::DecompressionError();
}

XPKDecompressor::Registry<SLZ3Decompressor> SLZ3Decompressor::_XPKregistration;
