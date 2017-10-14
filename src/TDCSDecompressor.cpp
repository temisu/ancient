/* Copyright (C) Teemu Suutari */

#include "TDCSDecompressor.hpp"

bool TDCSDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('TDCS');
}

std::unique_ptr<XPKDecompressor> TDCSDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<TDCSDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

TDCSDecompressor::TDCSDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

TDCSDecompressor::~TDCSDecompressor()
{
	// nothing needed
}

const std::string &TDCSDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-TDCS: LZ77-compressor";
	return name;
}

void TDCSDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	size_t bufOffset=0;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto read2Bits=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset+4>packedSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<4;i++) bufBitsContent=uint32_t(bufPtr[bufOffset++])|(bufBitsContent<<8);
			bufBitsLength=32;
		}
		uint8_t ret=bufBitsContent>>30;
		bufBitsContent<<=2;
		bufBitsLength-=2;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t rawSize=rawData.size();
	size_t destOffset=0;

	while (destOffset!=rawSize)
	{
		uint32_t distance=0;
		uint32_t count=0;
		uint32_t tmp;
		switch (read2Bits())
		{
			case 0:
			dest[destOffset++]=readByte();
			break;

			case 1:
			tmp=uint32_t(readByte())<<8;
			tmp|=uint32_t(readByte());
			count=(tmp&3)+3;
			distance=((tmp>>2)^0x3fff)+1;
			break;

			case 2:
			tmp=uint32_t(readByte())<<8;
			tmp|=uint32_t(readByte());
			count=(tmp&0xf)+3;
			distance=((tmp>>4)^0xfff)+1;
			break;

			case 3:
			distance=uint32_t(readByte())<<8;
			distance|=uint32_t(readByte());
			count=uint32_t(readByte())+3;
			if (!distance) throw Decompressor::DecompressionError();
			distance=(distance^0xffff)+1;
			break;
			
			default:
			throw Decompressor::DecompressionError();
		}
		if (count && distance)
		{
			if (distance>destOffset || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}
}

XPKDecompressor::Registry<TDCSDecompressor> TDCSDecompressor::_XPKregistration;
