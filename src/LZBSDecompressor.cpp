/* Copyright (C) Teemu Suutari */

#include "LZBSDecompressor.hpp"

bool LZBSDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('LZBS');
}

bool LZBSDecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> LZBSDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<LZBSDecompressor>(hdr,packedData,state);
}

LZBSDecompressor::LZBSDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (_packedData.size()<1) return;
	_isValid=true;
}

LZBSDecompressor::~LZBSDecompressor()
{
	// nothing needed
}

bool LZBSDecompressor::isValid() const
{
	return _isValid;
}

bool LZBSDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool LZBSDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &LZBSDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-LZBS: LZBS CyberYAFA compressor";
	return name;
}

bool LZBSDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=1;
	uint8_t bufBitsContent=0;
	uint32_t bufBitsLength=0;

	auto readBits=[&](uint32_t bits)->uint32_t
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<bits;i++)
		{
			if (!bufBitsLength)
			{
				if (bufOffset>=packedSize)
				{
					streamStatus=false;
					return 0;
				}
				bufBitsContent=bufPtr[bufOffset++];
				bufBitsLength=8;
			}
			ret|=(bufBitsContent>>7)<<i;
			bufBitsContent<<=1;
			bufBitsLength--;
		}
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	uint32_t bits=0,maxBits=uint32_t(bufPtr[0]);
	while (streamStatus && destOffset!=rawSize)
	{
		if (!readBits(1))
		{
			dest[destOffset++]=readBits(8);
		} else {
			uint32_t count=readBits(8)+2;
			if (count==2)
			{
				count=readBits(12);
				if (!count || destOffset+count>rawSize)
				{
					streamStatus=false;
				} else {
					for (uint32_t i=0;i<count;i++)
						dest[destOffset++]=readBits(8);
				}
			} else {
				while (destOffset>=(1U<<bits) && bits<maxBits) bits++;
				uint32_t distance=readBits(bits);

				if (!distance || distance>destOffset || destOffset+count>rawSize)
				{
					streamStatus=false;
				} else {
					for (uint32_t i=0;i<count;i++,destOffset++)
						dest[destOffset]=dest[destOffset-distance];
				}
			}
		}
	}

	return streamStatus && destOffset==rawSize;
}
