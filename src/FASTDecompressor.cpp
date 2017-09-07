/* Copyright (C) Teemu Suutari */

#include "FASTDecompressor.hpp"

bool FASTDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('FAST');
}

std::unique_ptr<XPKDecompressor> FASTDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<FASTDecompressor>(hdr,recursionLevel,packedData,state);
}

FASTDecompressor::FASTDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

FASTDecompressor::~FASTDecompressor()
{
	// nothing needed
}

bool FASTDecompressor::isValid() const
{
	return _isValid;
}

bool FASTDecompressor::verifyPacked() const
{
	return _isValid;
}

bool FASTDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

const std::string &FASTDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-FAST: Fast LZ77 compressor";
	return name;
}

bool FASTDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	size_t bufOffsetReverse=packedSize;
	uint16_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	size_t bufOffset=0;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffsetReverse<bufOffset+2)
			{
				streamStatus=false;
				return 0;
			}
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
		if (!streamStatus || bufOffset>=bufOffsetReverse)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[bufOffset++];
	};

	auto readShort=[&]()->uint16_t
	{
		if (!streamStatus) return 0;
		if (bufOffsetReverse<bufOffset+2)
		{
			streamStatus=false;
			return 0;
		}
		uint16_t ret=uint16_t(bufPtr[--bufOffsetReverse]);
		ret|=uint16_t(bufPtr[--bufOffsetReverse])<<8;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (streamStatus && destOffset!=rawSize)
	{
		if (!readBit())
		{
			if (destOffset<rawSize)
				dest[destOffset++]=readByte();
		} else {
			uint16_t ld=readShort();
			uint32_t count=18-(ld&0xf);
			uint32_t distance=ld>>4;
			if (!streamStatus || !distance || size_t(distance)>destOffset)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count&&destOffset<rawSize;i++,destOffset++)
					dest[destOffset]=dest[destOffset-distance];
			}
		}
	}

	return streamStatus && destOffset==rawSize;
}

static XPKDecompressor::Registry<FASTDecompressor> FASTRegistration;
