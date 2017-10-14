/* Copyright (C) Teemu Suutari */

#include "ILZRDecompressor.hpp"

bool ILZRDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('ILZR');
}

std::unique_ptr<XPKDecompressor> ILZRDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<ILZRDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

ILZRDecompressor::ILZRDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || packedData.size()<2)
		throw Decompressor::InvalidFormatError();
	_rawSize=_packedData.readBE16(0);
	if (!_rawSize) throw Decompressor::InvalidFormatError();
}

ILZRDecompressor::~ILZRDecompressor()
{
	// nothing needed
}

const std::string &ILZRDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-ILZR: Incremental Lempel-Ziv-Renau compressor";
	return name;
}

void ILZRDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	if (rawData.size()!=_rawSize) throw Decompressor::DecompressionError();

	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=2;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint32_t bits)->uint32_t
	{
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=(bufBitsContent<<8)|bufPtr[bufOffset++];
			bufBitsLength+=8;
		}

		uint32_t ret=(bufBitsContent>>(bufBitsLength-bits))&((1<<bits)-1);
		bufBitsLength-=bits;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	uint32_t bits=8;
	while (destOffset!=_rawSize)
	{
		if (readBits(1))
		{
			dest[destOffset++]=readBits(8);
		} else {
			while (destOffset>(1U<<bits)) bits++;
			uint32_t position=readBits(bits);
			uint32_t count=readBits(4)+3;

			if (position>=destOffset || destOffset+count>_rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[position+i];
		}
	}
}

XPKDecompressor::Registry<ILZRDecompressor> ILZRDecompressor::_XPKregistration;
