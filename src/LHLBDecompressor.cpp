/* Copyright (C) Teemu Suutari */

#include "LHLBDecompressor.hpp"

bool LHLBDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('LHLB');
}

std::unique_ptr<XPKDecompressor> LHLBDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<LHLBDecompressor>(hdr,recursionLevel,packedData,state);
}

LHLBDecompressor::LHLBDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

LHLBDecompressor::~LHLBDecompressor()
{
	// nothing needed
}

bool LHLBDecompressor::isValid() const
{
	return _isValid;
}

bool LHLBDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool LHLBDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &LHLBDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-LHLB: LZRW-compressor";
	return name;
}

bool LHLBDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=0;
	uint8_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
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
		uint8_t ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readBits=[&](uint32_t bits)->uint32_t
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<bits;i++) ret=(ret<<1)|readBit();
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	// Same logic as in Choloks pascal implementation
	// In his books LHLB is "almost" -lh1- (I'd assume the difference is in the metadata)
	uint32_t freq[633];
	uint32_t huff[950];
	int32_t sums[633];

	for (uint32_t i=0;i<317;i++)
	{
		freq[i]=1;
		huff[i]=316-i;
		sums[i]=-(i+1);
	}
	for (uint32_t i=0,j=0;i<316;i++,j+=2)
	{
		freq[i+317]=freq[j]+freq[j+1];
		huff[j+317]=i+317;
		huff[j+318]=i+317;
		sums[i+317]=j;
	}
	huff[949]=0;

	int32_t code=sums[632];
	while (streamStatus && destOffset!=rawSize)
	{
		code=sums[code+readBit()];
		if (code==-317) break;
		if (code<0)
		{
			if (freq[632]<0x8000)
			{
				uint32_t tmpIndex1=huff[code+317];
				do {
					freq[tmpIndex1]++;
					if (tmpIndex1==632) break;
					if (freq[tmpIndex1]>freq[tmpIndex1+1])
					{
						uint32_t tmpIndex2=tmpIndex1;
						do {
							tmpIndex2++;
						} while (tmpIndex2<632 && freq[tmpIndex1]>freq[tmpIndex2+1]);
						if (sums[tmpIndex1]>=0) huff[sums[tmpIndex1]+318]=tmpIndex2;
						if (sums[tmpIndex2]>=0) huff[sums[tmpIndex2]+318]=tmpIndex1;
						huff[sums[tmpIndex1]+317]=tmpIndex2;
						huff[sums[tmpIndex2]+317]=tmpIndex1;
						std::swap(freq[tmpIndex1],freq[tmpIndex2]);
						std::swap(sums[tmpIndex1],sums[tmpIndex2]);
						tmpIndex1=tmpIndex2;
					}
					tmpIndex1=huff[tmpIndex1+317];
				} while (tmpIndex1);
			}
			if (code>=-256)
			{
				dest[destOffset++]=-(code+1);
			} else {
				static const uint8_t distanceHighBits[256]={
					 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
					 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
					 2, 2, 2, 2, 2, 2, 2, 2,  2, 2, 2, 2, 2, 2, 2, 2,
					 3, 3, 3, 3, 3, 3, 3, 3,  3, 3, 3, 3, 3, 3, 3, 3,
					 4, 4, 4, 4, 4, 4, 4, 4,  5, 5, 5, 5, 5, 5, 5, 5,
					 6, 6, 6, 6, 6, 6, 6, 6,  7, 7, 7, 7, 7, 7, 7, 7,
					 8, 8, 8, 8, 8, 8, 8, 8,  9, 9, 9, 9, 9, 9, 9, 9,

					10,10,10,10,10,10,10,10, 11,11,11,11,11,11,11,11,
					12,12,12,12,13,13,13,13, 14,14,14,14,15,15,15,15,
					16,16,16,16,17,17,17,17, 18,18,18,18,19,19,19,19,
					20,20,20,20,21,21,21,21, 22,22,22,22,23,23,23,23,
					24,24,25,25,26,26,27,27, 28,28,29,29,30,30,31,31,
					32,32,33,33,34,34,35,35, 36,36,37,37,38,38,39,39,
					40,40,41,41,42,42,43,43, 44,44,45,45,46,46,47,47,
					48,49,50,51,52,53,54,55, 56,57,58,59,60,61,62,63};
				static const uint8_t distanceBits[16]={1,1,2,2,2,3,3,3,3,4,4,4,5,5,5,6};
				
				uint32_t tmp=readBits(8);
				uint32_t distance=uint32_t(distanceHighBits[tmp])<<6;
				uint32_t bits=distanceBits[tmp>>4];
				tmp=(tmp<<bits)|readBits(bits);
				distance|=tmp&63;
				uint32_t count=-(code+256);

				if (!distance || distance>destOffset || destOffset+count>rawSize)
				{
					streamStatus=false;
				} else {
					for (uint32_t i=0;i<count;i++,destOffset++)
						dest[destOffset]=dest[destOffset-distance];
				}
			}
			code=sums[632];
		}
	}

	return streamStatus && destOffset==rawSize;
}

XPKDecompressor::Registry<LHLBDecompressor> LHLBDecompressor::_XPKregistration;
