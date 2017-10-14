/* Copyright (C) Teemu Suutari */

#include "RLENDecompressor.hpp"

bool RLENDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('RLEN');
}

std::unique_ptr<XPKDecompressor> RLENDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<RLENDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

RLENDecompressor::RLENDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

RLENDecompressor::~RLENDecompressor()
{
	// nothing needed
}

const std::string &RLENDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-RLEN: RLE-compressor";
	return name;
}

void RLENDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	size_t packedSize=_packedData.size();

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (destOffset!=rawSize)
	{
		if (bufOffset==packedSize) throw Decompressor::DecompressionError();
		uint32_t count=uint32_t(bufPtr[bufOffset++]);
		if (count<128)
		{
			if (!count) throw Decompressor::DecompressionError();	// lets have this as error...
			if (bufOffset+count>packedSize || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=bufPtr[bufOffset++];
		} else {
			// I can see from different implementations that count=0x80 is buggy...
			// lets try to have it more or less correctly here
			count=256-count;
			if (bufOffset==packedSize || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			uint8_t ch=bufPtr[bufOffset++];
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=ch;
		}
	}
}

XPKDecompressor::Registry<RLENDecompressor> RLENDecompressor::_XPKregistration;
