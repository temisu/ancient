/* Copyright (C) Teemu Suutari */

#include "ILZRDecompressor.hpp"

bool ILZRDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('ILZR');
}

bool ILZRDecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> ILZRDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<ILZRDecompressor>(hdr,packedData,state);
}

ILZRDecompressor::ILZRDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	uint16_t tmp;
	if (!_packedData.readBE(0,tmp) || !tmp) return;
	_rawSize=tmp;
	_isValid=true;
}

ILZRDecompressor::~ILZRDecompressor()
{
	// nothing needed
}

bool ILZRDecompressor::isValid() const
{
	return _isValid;
}

bool ILZRDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool ILZRDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &ILZRDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-ILZR: Incremental Lempel-Ziv-Renau compressor";
	return name;
}

bool ILZRDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;
	if (rawData.size()!=_rawSize) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=2;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint32_t bits)->uint32_t
	{
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
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
	while (streamStatus && destOffset!=_rawSize)
	{
		if (readBits(1))
		{
			dest[destOffset++]=readBits(8);
		} else {
			while (destOffset>(1U<<bits)) bits++;
			uint32_t position=readBits(bits);
			uint32_t count=readBits(4)+3;

			if (position>=destOffset || destOffset+count>_rawSize)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count;i++,destOffset++)
					dest[destOffset]=dest[position+i];
			}
		}
	}

	return streamStatus && destOffset==_rawSize;
}
