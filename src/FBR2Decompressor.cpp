/* Copyright (C) Teemu Suutari */

#include "FBR2Decompressor.hpp"

bool FBR2Decompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('FBR2');
}

std::unique_ptr<XPKDecompressor> FBR2Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<FBR2Decompressor>(hdr,recursionLevel,packedData,state);
}

FBR2Decompressor::FBR2Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

FBR2Decompressor::~FBR2Decompressor()
{
	// nothing needed
}

bool FBR2Decompressor::isValid() const
{
	return _isValid;
}

bool FBR2Decompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool FBR2Decompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &FBR2Decompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-FBR2: FBR2 CyberYAFA compressor";
	return name;
}

bool FBR2Decompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;

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

	uint8_t mode=readByte();	
	while (streamStatus && destOffset!=rawSize)
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
			streamStatus=false;
			break;
		}

		count++;
		if (!streamStatus || destOffset+count>rawSize)
		{
			streamStatus=false;
		} else if (doCopy) {
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=readByte();
		} else {
			uint8_t repeatChar=readByte();
			for (uint32_t i=0;i<count;i++) dest[destOffset++]=repeatChar;
		}
	}

	return streamStatus && destOffset==rawSize;
}

static XPKDecompressor::Registry<FBR2Decompressor> FBR2Registration;
