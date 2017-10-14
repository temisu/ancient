/* Copyright (C) Teemu Suutari */

#include "FBR2Decompressor.hpp"

bool FBR2Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('FBR2');
}

std::unique_ptr<XPKDecompressor> FBR2Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<FBR2Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

FBR2Decompressor::FBR2Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();;
}

FBR2Decompressor::~FBR2Decompressor()
{
	// nothing needed
}

const std::string &FBR2Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-FBR2: FBR2 CyberYAFA compressor";
	return name;
}

void FBR2Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;

	auto readByte=[&]()->uint8_t
	{
		if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	uint8_t mode=readByte();	
	while (destOffset!=rawSize)
	{
		bool doCopy=false;
		uint32_t count=0;
		switch (mode)
		{
			case 33:
			count=uint32_t(readByte())<<24;
			count|=uint32_t(readByte())<<16;
			count|=uint32_t(readByte())<<8;
			count|=uint32_t(readByte());
			if (count>=0x8000'0000)
			{
				doCopy=true;
				count=-count;
			}
			break;

			case 67:
			count=uint32_t(readByte())<<8;
			count|=uint32_t(readByte());
			if (count>=0x8000)
			{
				doCopy=true;
				count=0x10000-count;
			}
			break;

			case 100:
			count=uint32_t(readByte());
			if (count>=0x80)
			{
				doCopy=true;
				count=0x100-count;
			}
			break;

			default:
			throw Decompressor::DecompressionError();
		}

		count++;
		if (destOffset+count>rawSize) throw Decompressor::DecompressionError();
		if (doCopy) {
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=readByte();
		} else {
			uint8_t repeatChar=readByte();
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=repeatChar;
		}
	}
}

XPKDecompressor::Registry<FBR2Decompressor> FBR2Decompressor::_XPKregistration;
