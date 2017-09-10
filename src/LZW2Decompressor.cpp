/* Copyright (C) Teemu Suutari */

#include "LZW2Decompressor.hpp"

bool LZW2Decompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('LZW2') || hdr==FourCC('LZW3');
}

std::unique_ptr<XPKDecompressor> LZW2Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<LZW2Decompressor>(hdr,recursionLevel,packedData,state);
}

LZW2Decompressor::LZW2Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_ver=(hdr==FourCC('LZW2'))?2:3;
	_isValid=true;
}

LZW2Decompressor::~LZW2Decompressor()
{
	// nothing needed
}

bool LZW2Decompressor::isValid() const
{
	return _isValid;
}

bool LZW2Decompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool LZW2Decompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &LZW2Decompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name2="XPK-LZW2: LZW2 CyberYAFA compressor";
	static std::string name3="XPK-LZW3: LZW3 CyberYAFA compressor";
	return (_ver==2)?name2:name3;
}

bool LZW2Decompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffset+3>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=uint32_t(bufPtr[bufOffset++])<<24;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<16;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<8;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++]);
			bufBitsLength=32;
		}
		uint8_t ret=bufBitsContent&1;
		bufBitsContent>>=1;
		bufBitsLength--;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (!streamStatus || bufOffset>=packedSize)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (streamStatus && destOffset!=rawSize)
	{
		if (!readBit())
		{
			dest[destOffset++]=readByte();
		} else {
			uint32_t distance=uint32_t(readByte())<<8;
			distance|=uint32_t(readByte());
			if (!distance) break;
			distance=65536-distance;
			uint32_t count=uint32_t(readByte())+4;

			if (distance>destOffset || destOffset+count>rawSize)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count;i++,destOffset++)
					dest[destOffset]=dest[destOffset-distance];
			}
		}
	}

	return streamStatus && destOffset==rawSize;
}

XPKDecompressor::Registry<LZW2Decompressor> LZW2Decompressor::_XPKregistration;
