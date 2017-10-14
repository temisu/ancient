/* Copyright (C) Teemu Suutari */

#include "FASTDecompressor.hpp"

bool FASTDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('FAST');
}

std::unique_ptr<XPKDecompressor> FASTDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<FASTDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

FASTDecompressor::FASTDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

FASTDecompressor::~FASTDecompressor()
{
	// nothing needed
}

const std::string &FASTDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-FAST: Fast LZ77 compressor";
	return name;
}

void FASTDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	size_t bufOffsetReverse=packedSize;
	uint16_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	size_t bufOffset=0;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffsetReverse<bufOffset+2) throw Decompressor::DecompressionError();
			bufBitsContent=uint16_t(bufPtr[--bufOffsetReverse]);
			bufBitsContent|=uint16_t(bufPtr[--bufOffsetReverse])<<8;
			bufBitsLength=16;
		}
		uint8_t ret=bufBitsContent>>15;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (bufOffset>=bufOffsetReverse) throw Decompressor::DecompressionError();
		return bufPtr[bufOffset++];
	};

	auto readShort=[&]()->uint16_t
	{
		if (bufOffsetReverse<bufOffset+2) throw Decompressor::DecompressionError();
		uint16_t ret=uint16_t(bufPtr[--bufOffsetReverse]);
		ret|=uint16_t(bufPtr[--bufOffsetReverse])<<8;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (destOffset!=rawSize)
	{
		if (!readBit())
		{
			if (destOffset<rawSize)
				dest[destOffset++]=readByte();
		} else {
			uint16_t ld=readShort();
			uint32_t count=18-(ld&0xf);
			uint32_t distance=ld>>4;
			if (!distance || size_t(distance)>destOffset) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count&&destOffset<rawSize;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}
}

XPKDecompressor::Registry<FASTDecompressor> FASTDecompressor::_XPKregistration;
