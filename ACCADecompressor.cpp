/* Copyright (C) Teemu Suutari */

#include "ACCADecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool ACCADecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('ACCA');
}

ACCADecompressor::ACCADecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

ACCADecompressor::~ACCADecompressor()
{
	// nothing needed
}

bool ACCADecompressor::isValid() const
{
	return _isValid;
}

bool ACCADecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool ACCADecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &ACCADecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-ACCA: Andre's Code Compression Algorithm";
	return name;
}

bool ACCADecompressor::decompress(Buffer &rawData,const Buffer &previousData)
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
			static const uint8_t staticBytes[16]={
				0x00,0x01,0x02,0x03,0x04,0x08,0x10,0x20,
				0x40,0x55,0x60,0x80,0xaa,0xc0,0xe0,0xff};

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
				case 14:
				count+=3;
				doRLE=true;
				break;

				case 1:
				count=(count|(uint32_t(readByte())<<4))+19;
				repeatChar=readByte();
				doRLE=true;
				break;

				case 2:
				repeatChar=staticBytes[count];
				count=2;
				doRLE=true;
				break;

				case 15:
				distance=(count|(uint32_t(readByte())<<4))+3;
				count=uint32_t(readByte())+14;
				break;

				default: /* 3 to 13 */
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
