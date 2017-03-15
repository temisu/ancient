/* Copyright (C) Teemu Suutari */

#include "CRMDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include "DLTADecode.hpp"

bool CRMDecompressor::detectHeader(uint32_t hdr)
{
	switch (hdr)
	{
		case FourCC('CrM!'):
		case FourCC('CrM2'):
		case FourCC('Crm!'):
		case FourCC('Crm2'):
		return true;

		default:
		return false;
	}
}

bool CRMDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('CRM2') || hdr==FourCC('CRMS');
}

CRMDecompressor::CRMDecompressor(const Buffer &packedData) :
	Decompressor(packedData)
{
	if (packedData.size()<20) return;
	uint32_t hdr;
	if (!packedData.readBE(0,hdr)) return;
	if (!detectHeader(hdr)) return;

	if (!packedData.readBE(6,_rawSize)) return;
	if (!_rawSize) return;
	if (!packedData.readBE(10,_packedSize)) return;
	if (_packedSize+14>packedData.size()) return;
	if (((hdr>>8)&0xff)=='m') _isSampled=true;
	if ((hdr&0xff)=='2') _isLZH=true;
	_isValid=true;
}

CRMDecompressor::CRMDecompressor(uint32_t hdr,const Buffer &packedData) :
	CRMDecompressor(packedData)
{
	_isXPKDelta=(hdr==FourCC('CRMS'));
}

CRMDecompressor::~CRMDecompressor()
{
	// nothing needed
}

bool CRMDecompressor::isValid() const
{
	return _isValid;
}

bool CRMDecompressor::verifyPacked() const
{
	// no checksum whatsoever
	return _isValid;
}

bool CRMDecompressor::verifyRaw(const Buffer &rawData) const
{
	// no CRC
	return _isValid;
}

const std::string &CRMDecompressor::getName() const
{
	if (!_isValid) return Decompressor::getName();
	static std::string names[4]={
		"CrM!: Crunch-Mania standard-mode",
		"Crm!: Crunch-Mania standard-mode, sampled",
		"CrM2: Crunch-Mania LZH-mode",
		"Crm2: Crunch-Mania LZH-mode, sampled"};
	return names[(_isLZH?2:0)+(_isSampled?1:0)];
}

const std::string &CRMDecompressor::getSubName() const
{
	// the XPK-id is not used in decompressing process.
	// This means we can have frankenstein configurations
	// Least we can do is to report them...
	if (!_isValid) return Decompressor::getName();
	static std::string names[8]={
		"XPK-CRM2: Crunch-Mania standard-mode",
		"XPK-CRM2: Crunch-Mania standard-mode, sampled",
		"XPK-CRM2: Crunch-Mania LZH-mode",			// good config 1
		"XPK-CRM2: Crunch-Mania LZH-mode, sampled",
		"XPK-CRMS: Crunch-Mania standard-mode",
		"XPK-CRMS: Crunch-Mania standard-mode, sampled",
		"XPK-CRMS: Crunch-Mania LZH-mode",
		"XPK-CRMS: Crunch-Mania LZH-mode, sampled"};		// good config 2
	return names[(_isXPKDelta?4:0)|(_isLZH?2:0)|(_isSampled?1:0)];
}

size_t CRMDecompressor::getPackedSize() const
{
	if (!_isValid) return 0;
	return _packedSize+14;
}

size_t CRMDecompressor::getRawSize() const
{
	if (!_isValid) return 0;
	return _rawSize;
}

bool CRMDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_rawSize) return false;

	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_packedSize+14-6;

	// There are empty bits?!? at the start of the stream. take them out
	uint32_t originalBitsContent;
	if (!_packedData.readBE(bufOffset,originalBitsContent)) return false;
	uint16_t originalShift;
	if (!_packedData.readBE(bufOffset+4,originalShift)) return false;
	uint8_t bufBitsLength=originalShift+16;
	uint32_t bufBitsContent=originalBitsContent>>(16-originalShift);

	// streamreader
	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (bufOffset<=14)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=uint32_t(bufPtr[--bufOffset]);
			bufBitsLength=8;
		}
		uint8_t ret=bufBitsContent&1;
		bufBitsContent>>=1;
		bufBitsLength--;
		return ret;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		if (!streamStatus) return 0;
		while (bufBitsLength<count)
		{
			if (bufOffset<=14)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent|=uint32_t(bufPtr[--bufOffset])<<bufBitsLength;
			bufBitsLength+=8;
		}
		uint32_t ret=bufBitsContent&((1<<count)-1);
		bufBitsContent>>=count;
		bufBitsLength-=count;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=_rawSize;

	if (_isLZH)
	{
		typedef HuffmanDecoder<int32_t,-1,0> CRMHuffmanDecoder;

		auto readHuffmanTable=[&](CRMHuffmanDecoder &dec,uint32_t codeLength)
		{
			uint32_t maxDepth=readBits(4);
			if (!maxDepth)
			{
				streamStatus=false;
				return;
			}
			uint32_t lengthTable[maxDepth];
			for (uint32_t i=0;i<maxDepth;i++)
				lengthTable[i]=readBits(std::min(i+1,codeLength));
			uint32_t code=0;
			for (uint32_t depth=1;depth<=maxDepth;depth++)
			{
				for (uint32_t i=0;i<lengthTable[depth-1];i++)
				{
						int32_t value=readBits(codeLength);
						dec.insert(HuffmanCode<int32_t>{depth,code>>(maxDepth-depth),value});
						code+=1<<(maxDepth-depth);
				}
			}
			if (streamStatus) streamStatus=dec.isValid();
		};


		do {
			CRMHuffmanDecoder lengthDecoder,distanceDecoder;
			readHuffmanTable(lengthDecoder,9);
			readHuffmanTable(distanceDecoder,4);

			uint32_t items=readBits(16)+1;
			for (uint32_t i=0;i<items&&streamStatus;i++)
			{
				int32_t count=lengthDecoder.decode(readBit);
				if (count==-1)
				{
					streamStatus=false;
					break;
				}
				if (count&0x100)
				{
					// this is literal, not count
					if (!destOffset)
					{
						streamStatus=false;
					} else {
						dest[--destOffset]=count;
					}
				} else {
					count+=3;

					int32_t distanceBits=distanceDecoder.decode(readBit);
					if (distanceBits==-1)
					{
						streamStatus=false;
						break;
					}
					uint32_t distance;
					if (!distanceBits)
					{
						distance=readBits(1)+1;
					} else {
						distance=(readBits(distanceBits)|(1<<distanceBits))+1;
					}
					if (destOffset<size_t(count) || destOffset+distance>_rawSize)
					{
						streamStatus=false;
					} else {
						distance+=destOffset;
						for (int32_t i=0;i<count;i++) dest[--destOffset]=dest[--distance];
					}
				}
			}
		} while (streamStatus && readBit());
	} else {
		HuffmanDecoder<uint8_t,0xff,3> lengthDecoder
		{
			HuffmanCode<uint8_t>{1,0b000,0},
			HuffmanCode<uint8_t>{2,0b010,1},
			HuffmanCode<uint8_t>{3,0b110,2},
			HuffmanCode<uint8_t>{3,0b111,3}
		};

		HuffmanDecoder<uint8_t,0xff,2> distanceDecoder
		{
			HuffmanCode<uint8_t>{1,0b00,0},
			HuffmanCode<uint8_t>{2,0b10,1},
			HuffmanCode<uint8_t>{2,0b11,2}
		};

		while (streamStatus)
		{
			if (!destOffset) break;

			if (readBit())
			{
				dest[--destOffset]=readBits(8);
			} else {
				uint8_t lengthIndex=lengthDecoder.decode(readBit);

				static const uint8_t lengthBits[4]={1,2,4,8};
				static const uint32_t lengthAdditions[4]={2,4,8,24};
				uint32_t count=readBits(lengthBits[lengthIndex])+lengthAdditions[lengthIndex];
				if (count==23)
				{
					if (readBit())
					{
						count=readBits(5)+15;
					} else {
						count=readBits(14)+15;
					}
					if (count>destOffset)
					{
						streamStatus=false;
						break;
					} else {
						for (uint32_t i=0;i<count;i++)
							dest[--destOffset]=readBits(8);
					}
				} else {
					if (count>23) count--;

					uint8_t distanceIndex=distanceDecoder.decode(readBit);
				
					static const uint8_t distanceBits[3]={9,5,14};
					static const uint32_t distanceAdditions[3]={32,0,544};
					uint32_t distance=readBits(distanceBits[distanceIndex])+distanceAdditions[distanceIndex];

					if (!distance || destOffset<count || destOffset+distance>_rawSize)
					{
						streamStatus=false;
					} else {
						distance+=destOffset;
						for (uint32_t i=0;i<count;i++) dest[--destOffset]=dest[--distance];
					}
				}
			}
		}
	}

	bool ret=(streamStatus && !destOffset);
	if (ret && _isSampled)
		DLTADecode::decode(rawData,rawData,0,_rawSize);

	return ret;
}
