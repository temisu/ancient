/* Copyright (C) Teemu Suutari */

#include <string.h>

#include "NUKEDecompressor.hpp"
#include "DLTADecode.hpp"

bool NUKEDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('NUKE') || hdr==FourCC('DUKE');
}

std::unique_ptr<XPKDecompressor> NUKEDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<NUKEDecompressor>(hdr,recursionLevel,packedData,state);
}

NUKEDecompressor::NUKEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (hdr==FourCC('DUKE')) _isDUKE=true;
	_isValid=true;
}

NUKEDecompressor::~NUKEDecompressor()
{
	// nothing needed
}

bool NUKEDecompressor::isValid() const
{
	return _isValid;
}

bool NUKEDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool NUKEDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &NUKEDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string nameN="XPK-NUKE: LZ77-compressor";
	static std::string nameD="XPK-DUKE: LZ77-compressor with delta encoding";
	return (_isDUKE)?nameD:nameN;
}

bool NUKEDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	// there are 2 streams, reverse stream for bytes and
	// normal stream for bits, the bit stream is divided
	// into single bit, 2 bit, 4 bit and random accumulator
	size_t bufOffset=0;
	uint16_t bufBits1Content=0;
	uint8_t bufBits1Length=0;
	uint16_t bufBits2Content=0;
	uint8_t bufBits2Length=0;
	uint32_t bufBits4Content=0;
	uint8_t bufBits4Length=0;
	uint32_t bufBitsXContent=0;
	uint8_t bufBitsXLength=0;
	size_t bufOffsetReverse=packedSize;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBits1Length)
		{
			if (bufOffset+1>=bufOffsetReverse)
			{
				streamStatus=false;
				return 0;
			}
			bufBits1Content=uint16_t(bufPtr[bufOffset++])<<8;
			bufBits1Content|=uint16_t(bufPtr[bufOffset++]);
			bufBits1Length=16;
		}
		uint8_t ret=bufBits1Content>>15;
		bufBits1Content<<=1;
		bufBits1Length--;
		return ret;
	};

	auto read2Bits=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBits2Length)
		{
			if (bufOffset+1>=bufOffsetReverse)
			{
				streamStatus=false;
				return 0;
			}
			bufBits2Content=uint16_t(bufPtr[bufOffset++])<<8;
			bufBits2Content|=uint16_t(bufPtr[bufOffset++]);
			bufBits2Length=16;
		}
		uint8_t ret=bufBits2Content>>14;
		bufBits2Content<<=2;
		bufBits2Length-=2;
		return ret;
	};

	auto read4Bits=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBits4Length)
		{
			if (bufOffset+3>=bufOffsetReverse)
			{
				streamStatus=false;
				return 0;
			}
			bufBits4Content=uint32_t(bufPtr[bufOffset++])<<24;
			bufBits4Content|=uint32_t(bufPtr[bufOffset++])<<16;
			bufBits4Content|=uint32_t(bufPtr[bufOffset++])<<8;
			bufBits4Content|=uint32_t(bufPtr[bufOffset++]);
			bufBits4Length=32;
		}
		uint8_t ret=bufBits4Content&0xf;
		bufBits4Content>>=4;
		bufBits4Length-=4;
		return ret;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		if (!streamStatus) return 0;
		if (bufBitsXLength<count)
		{
			if (bufOffset+1>=bufOffsetReverse)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsXContent<<=16;
			bufBitsXContent|=uint16_t(bufPtr[bufOffset++])<<8;
			bufBitsXContent|=uint16_t(bufPtr[bufOffset++]);
			bufBitsXLength+=16;
		}
		uint32_t ret=(bufBitsXContent>>(bufBitsXLength-count))&((1<<count)-1);
		bufBitsXLength-=count;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (!streamStatus || bufOffsetReverse<=bufOffset)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[--bufOffsetReverse];
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	while (streamStatus)
	{
		if (!readBit())
		{
			uint32_t count=0;
			if (readBit())
			{
				count=1;
			} else {
				uint32_t tmp;
				do {
					tmp=read2Bits();
					if (tmp) count+=5-tmp;
						else count+=3;
				} while (!tmp);
			}
			if (destOffset+count>rawSize)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count;i++) dest[destOffset++]=readByte();
			}
		}
		if (destOffset==rawSize) break;
		uint32_t distanceIndex=read4Bits();
		static const uint8_t distanceBits[16]={
			4,6,8,9,
			4,7,9,11,13,14,
			5,7,9,11,13,14};
		static const uint32_t distanceAdditions[16]={
			0,0x10,0x50,0x150,
			0,0x10,0x90,0x290,0xa90,0x2a90,
			0,0x20,0xa0,0x2a0,0xaa0,0x2aa0};
		uint32_t distance=readBits(distanceBits[distanceIndex])+distanceAdditions[distanceIndex];
		uint32_t count=(distanceIndex<4)?2:(distanceIndex<10)?3:0;
		if (!count)
		{
			count=read2Bits();
			if (!count)
			{
				count=3+3;
				uint32_t tmp;
				do {
					tmp=read4Bits();
					if (tmp) count+=16-tmp;
						else count+=15;
				} while (!tmp);
			} else count=3+4-count;
		}
		if (!streamStatus || !distance || size_t(distance)>destOffset || destOffset+count>rawSize)
		{
			streamStatus=false;
		} else {
			for (uint32_t i=0;i<count;i++,destOffset++)
				dest[destOffset]=dest[destOffset-distance];
		}
	}
	bool ret=(streamStatus && destOffset==rawSize);
	if (ret && _isDUKE)
		DLTADecode::decode(rawData,rawData,0,rawSize);
	return ret;
}

static XPKDecompressor::Registry<NUKEDecompressor> NUKERegistration;
