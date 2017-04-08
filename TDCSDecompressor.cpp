/* Copyright (C) Teemu Suutari */

#include "TDCSDecompressor.hpp"

bool TDCSDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('TDCS');
}

TDCSDecompressor::TDCSDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

TDCSDecompressor::~TDCSDecompressor()
{
	// nothing needed
}

bool TDCSDecompressor::isValid() const
{
	return _isValid;
}

bool TDCSDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool TDCSDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &TDCSDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-TDCS: TDCS LZ77-compressor";
	return name;
}

bool TDCSDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	size_t bufOffset=0;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto read2Bits=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffset+4>packedSize)
			{
				streamStatus=false;
				return 0;
			}
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
		if (bufOffset>=packedSize)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t rawSize=rawData.size();
	size_t destOffset=0;

	while (streamStatus && destOffset<rawSize)
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
			if (!distance) streamStatus=false;
			distance=(distance^0xffff)+1;
			break;
			
			default:
			streamStatus=false;
			break;
		}
		if (streamStatus && count && distance)
		{
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
