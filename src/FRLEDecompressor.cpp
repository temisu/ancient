/* Copyright (C) Teemu Suutari */

#include "FRLEDecompressor.hpp"

bool FRLEDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('FRLE');
}

std::unique_ptr<XPKDecompressor> FRLEDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<FRLEDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

FRLEDecompressor::FRLEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

FRLEDecompressor::~FRLEDecompressor()
{
	// nothing needed
}

const std::string &FRLEDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-FRLE: RLE-compressor";
	return name;
}

void FRLEDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
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

		auto countMod=[](uint32_t count)->uint32_t
		{
			return (32-(count&0x1f))+(count&0x60);
		};

		uint32_t count=uint32_t(bufPtr[bufOffset++]);

		if (count<128)
		{
			count=countMod(count);
			if (bufOffset+count>packedSize || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=bufPtr[bufOffset++];
		} else {
			count=countMod(count)+1;
			if (bufOffset==packedSize || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			uint8_t ch=bufPtr[bufOffset++];
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=ch;
		}
	}
}

XPKDecompressor::Registry<FRLEDecompressor> FRLEDecompressor::_XPKregistration;
