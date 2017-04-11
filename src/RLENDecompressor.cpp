/* Copyright (C) Teemu Suutari */

#include "RLENDecompressor.hpp"

bool RLENDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('RLEN');
}

std::unique_ptr<XPKDecompressor> RLENDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<RLENDecompressor>(hdr,packedData,state);
}

RLENDecompressor::RLENDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

RLENDecompressor::~RLENDecompressor()
{
	// nothing needed
}

bool RLENDecompressor::isValid() const
{
	return _isValid;
}

bool RLENDecompressor::verifyPacked() const
{
	return _isValid;
}

bool RLENDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

const std::string &RLENDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-RLEN: RLE compressor";
	return name;
}

bool RLENDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	size_t packedSize=_packedData.size();

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (destOffset<rawSize)
	{
		if (bufOffset==packedSize) break;
		uint32_t count=uint32_t(bufPtr[bufOffset++]);
		if (count<128)
		{
			if (!count) break;	// lets have this as error...
			if (bufOffset+count>packedSize || destOffset+count>rawSize) break;
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=bufPtr[bufOffset++];
		} else {
			// I can see from different implementations that count=0x80 is buggy...
			// lets try to have it more or less correctly here
			count=256-count;
			if (bufOffset==packedSize || destOffset+count>rawSize) break;
			uint8_t ch=bufPtr[bufOffset++];
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=ch;
		}
	}

	return destOffset==rawSize;
}
