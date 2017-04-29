/* Copyright (C) Teemu Suutari */

#include "FRLEDecompressor.hpp"

bool FRLEDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('FRLE');
}

bool FRLEDecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> FRLEDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<FRLEDecompressor>(hdr,packedData,state);
}

FRLEDecompressor::FRLEDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

FRLEDecompressor::~FRLEDecompressor()
{
	// nothing needed
}

bool FRLEDecompressor::isValid() const
{
	return _isValid;
}

bool FRLEDecompressor::verifyPacked() const
{
	return _isValid;
}

bool FRLEDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

const std::string &FRLEDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-FRLE: RLE-compressor";
	return name;
}

bool FRLEDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
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

		auto countMod=[](uint32_t count)->uint32_t
		{
			return (32-(count&0x1f))+(count&0x60);
		};

		uint32_t count=uint32_t(bufPtr[bufOffset++]);

		if (count<128)
		{
			count=countMod(count);
			if (bufOffset+count>packedSize || destOffset+count>rawSize) break;
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=bufPtr[bufOffset++];
		} else {
			count=countMod(count)+1;
			if (bufOffset==packedSize || destOffset+count>rawSize) break;
			uint8_t ch=bufPtr[bufOffset++];
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=ch;
		}
	}

	return destOffset==rawSize;
}
