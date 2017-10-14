/* Copyright (C) Teemu Suutari */

#include "LZW4Decompressor.hpp"

bool LZW4Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('LZW4');
}

std::unique_ptr<XPKDecompressor> LZW4Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LZW4Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

LZW4Decompressor::LZW4Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

LZW4Decompressor::~LZW4Decompressor()
{
	// nothing needed
}

const std::string &LZW4Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-LZW4: LZW4 CyberYAFA compressor";
	return name;
}

void LZW4Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset+3>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=uint32_t(bufPtr[bufOffset++])<<24;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<16;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<8;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++]);
			bufBitsLength=32;
		}
		uint8_t ret=bufBitsContent>>31;
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
			uint32_t distance=uint32_t(readByte())<<8;
			distance|=uint32_t(readByte());
			if (!distance) break;
			distance=65536-distance;
			uint32_t count=uint32_t(readByte())+3;

			if (distance>destOffset || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}

	if (destOffset!=rawSize) throw Decompressor::DecompressionError();
}

XPKDecompressor::Registry<LZW4Decompressor> LZW4Decompressor::_XPKregistration;
