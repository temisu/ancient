/* Copyright (C) Teemu Suutari */

#include "LIN1Decompressor.hpp"

bool LIN1Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('LIN1') || hdr==FourCC('LIN3');
}

std::unique_ptr<XPKDecompressor> LIN1Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<LIN1Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

LIN1Decompressor::LIN1Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	_ver=(hdr==FourCC('LIN1'))?1:3;
	if (packedData.size()<5) throw Decompressor::InvalidFormatError();

	uint32_t tmp=packedData.readBE32(0);
	if (tmp) throw Decompressor::InvalidFormatError();	// password set
}

LIN1Decompressor::~LIN1Decompressor()
{
	// nothing needed
}

const std::string &LIN1Decompressor::getSubName() const noexcept
{
	static std::string name1="XPK-LIN1: LIN1 LINO packer";
	static std::string name3="XPK-LIN3: LIN3 LINO packer";
	return (_ver==1)?name1:name3;
}

void LIN1Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=5;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint8_t bits)->uint8_t
	{
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
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
		if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (destOffset!=rawSize)
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

			if (distance>destOffset) throw Decompressor::DecompressionError();
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}

	if (destOffset!=rawSize) throw Decompressor::DecompressionError();
}

XPKDecompressor::Registry<LIN1Decompressor> LIN1Decompressor::_XPKregistration;
