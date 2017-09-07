/* Copyright (C) Teemu Suutari */

#include "LIN1Decompressor.hpp"

bool LIN1Decompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('LIN1') || hdr==FourCC('LIN3');
}

std::unique_ptr<XPKDecompressor> LIN1Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<LIN1Decompressor>(hdr,recursionLevel,packedData,state);
}

LIN1Decompressor::LIN1Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_ver=(hdr==FourCC('LIN1'))?1:3;
	if (packedData.size()<5) return;

	uint32_t tmp;
	if (!packedData.readBE(0,tmp) || tmp) return;	// password set

	_isValid=true;
}

LIN1Decompressor::~LIN1Decompressor()
{
	// nothing needed
}

bool LIN1Decompressor::isValid() const
{
	return _isValid;
}

bool LIN1Decompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool LIN1Decompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &LIN1Decompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name1="XPK-LIN1: LIN1 LINO packer";
	static std::string name3="XPK-LIN3: LIN3 LINO packer";
	return (_ver==1)?name1:name3;
}

bool LIN1Decompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=5;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint8_t bits)->uint8_t
	{
		if (!streamStatus) return 0;
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent<<=8;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++]);
			bufBitsLength+=8;
		}
		uint8_t ret=(bufBitsContent>>(bufBitsLength-bits))&((1<<bits)-1);
		bufBitsLength-=bits;
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
		if (!readBits(1))
		{
			dest[destOffset++]=readByte()^0x55;
		} else {
			uint32_t count=3;
			if (readBits(1))
			{
				count=readBits(2);
				if (count==3)
				{
					count=readBits(3);
					if (count==7)
					{
						count=readBits(4);
						if (count==15)
						{
							count=readByte();
							if (count==0xff) break;
							count+=3;
						} else count+=14;
					} else count+=7;
				} else count+=4;
			}
			uint32_t distance;
			switch (readBits(2))
			{
				case 0:
				distance=readByte()+1;
				break;

				case 1:
				distance=uint32_t(readBits(2))<<8;
				distance|=readByte();
				distance+=0x101;
				break;

				case 2:
				distance=uint32_t(readBits(4))<<8;
				distance|=readByte();
				distance+=0x501;
				break;

				case 3:
				distance=uint32_t(readBits(6))<<8;
				distance|=readByte();
				distance+=0x1501;
				break;
			}

			// buggy compressors
			if (destOffset+count>rawSize) count=uint32_t(rawSize-destOffset);
			if (!count) break;

			if (distance>destOffset)
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

static XPKDecompressor::Registry<LIN1Decompressor> LIN1Registration;
