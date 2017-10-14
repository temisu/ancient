/* Copyright (C) Teemu Suutari */

#include "CBR0Decompressor.hpp"

bool CBR0Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('CBR0');
}

std::unique_ptr<XPKDecompressor> CBR0Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<CBR0Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

CBR0Decompressor::CBR0Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

CBR0Decompressor::~CBR0Decompressor()
{
	// nothing needed
}

const std::string &CBR0Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-CBR0: RLE-compressor";
	return name;
}

void CBR0Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool veridy)
{
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	size_t packedSize=_packedData.size();

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	// barely different than RLEN, however the count is well defined here.
	while (destOffset<rawSize)
	{
		if (bufOffset==packedSize) break;
		uint32_t count=uint32_t(bufPtr[bufOffset++]);
		if (count<128)
		{
			count++;
			if (bufOffset+count>packedSize || destOffset+count>rawSize) break;
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=bufPtr[bufOffset++];
		} else {
			count=257-count;
			if (bufOffset==packedSize || destOffset+count>rawSize) break;
			uint8_t ch=bufPtr[bufOffset++];
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=ch;
		}
	}

	if (destOffset!=rawSize) throw Decompressor::DecompressionError();
}

XPKDecompressor::Registry<CBR0Decompressor> CBR0Decompressor::_XPKregistration;
