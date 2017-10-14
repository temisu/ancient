/* Copyright (C) Teemu Suutari */

#include "LZW5Decompressor.hpp"

bool LZW5Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('LZW5');
}

std::unique_ptr<XPKDecompressor> LZW5Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LZW5Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

LZW5Decompressor::LZW5Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

LZW5Decompressor::~LZW5Decompressor()
{
	// nothing needed
}

const std::string &LZW5Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-LZW5: LZW5 CyberYAFA compressor";
	return name;
}

void LZW5Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
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
			if (bufOffset+3>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=uint32_t(bufPtr[bufOffset++])<<24;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<16;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<8;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++]);
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
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (destOffset!=rawSize)
	{
		uint32_t distance,count;
		bool doCopy=false,doExit=false;

		auto readld=[&]()->uint32_t
		{
			uint32_t ret=uint32_t(readByte())<<8;
			ret|=uint32_t(readByte());
			if (!ret) doExit=true;
			return ret;
		};

		switch (read2Bits())
		{
			case 0:
			dest[destOffset++]=readByte();
			break;
			
			case 1:
			distance=readld();
			count=(distance&3)+2;
			distance=0x4000-(distance>>2);
			doCopy=true;
			break;

			case 2:
			distance=readld();
			count=(distance&15)+2;
			distance=0x1000-(distance>>4);
			doCopy=true;
			break;

			case 3:
			distance=readld();
			count=uint32_t(readByte())+3;
			distance=0x10000-distance;
			doCopy=true;
			break;
			
			default:
			throw Decompressor::DecompressionError();
		}
		if (doExit) break;
		if (doCopy)
		{
			if (distance>destOffset || destOffset+count>rawSize) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}

	if (destOffset!=rawSize) throw Decompressor::DecompressionError();
}

XPKDecompressor::Registry<LZW5Decompressor> LZW5Decompressor::_XPKregistration;
