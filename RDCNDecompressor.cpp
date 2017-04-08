/* Copyright (C) Teemu Suutari */

#include "RDCNDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool RDCNDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('RDCN');
}

RDCNDecompressor::RDCNDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

RDCNDecompressor::~RDCNDecompressor()
{
	// nothing needed
}

bool RDCNDecompressor::isValid() const
{
	return _isValid;
}

bool RDCNDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool RDCNDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &RDCNDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-RDCN: Ross Data Compression";
	return name;
}

bool RDCNDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	uint16_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffset+1>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=uint16_t(bufPtr[bufOffset++])<<8;
			bufBitsContent|=uint16_t(bufPtr[bufOffset++]);
			bufBitsLength=16;
		}
		uint8_t ret=bufBitsContent>>15;
		bufBitsContent<<=1;
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
			uint8_t tmp=readByte();
			uint32_t count=tmp&0xf;
			uint32_t code=tmp>>4;
			uint32_t distance=0;
			uint8_t repeatChar=0;
			bool doRLE=false;
			switch (code)
			{
				case 0:
				repeatChar=readByte();
				count+=3;
				doRLE=true;
				break;

				case 1:
				count=(count|(uint32_t(readByte())<<4))+19;
				repeatChar=readByte();
				doRLE=true;
				break;

				case 2:
				distance=(count|(uint32_t(readByte())<<4))+3;
				count=uint32_t(readByte())+16;
				break;

				default: /* 3 to 15 */
				distance=(count|(uint32_t(readByte())<<4))+3;
				count=code;
				break;
			}
			if (doRLE)
			{
				if (!streamStatus || destOffset+count>rawSize)
				{
					streamStatus=false;
				} else {
					for (uint32_t i=0;i<count;i++) dest[destOffset++]=repeatChar;
				}
			} else {
				if (distance>destOffset || destOffset+count>rawSize)
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
