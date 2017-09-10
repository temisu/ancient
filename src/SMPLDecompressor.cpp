/* Copyright (C) Teemu Suutari */

#include "SMPLDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool SMPLDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('SMPL');
}

std::unique_ptr<XPKDecompressor> SMPLDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<SMPLDecompressor>(hdr,recursionLevel,packedData,state);
}

SMPLDecompressor::SMPLDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (packedData.size()<2) return;

	uint16_t tmp;
	if (!packedData.readBE(0,tmp)) return;
	if (tmp!=1) return;

	_isValid=true;
}

SMPLDecompressor::~SMPLDecompressor()
{
	// nothing needed
}

bool SMPLDecompressor::isValid() const
{
	return _isValid;
}

bool SMPLDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool SMPLDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &SMPLDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-SMPL: Huffman compressor with delta encoding";
	return name;
}

bool SMPLDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	size_t bufOffset=2;
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
		return (bufBitsContent>>(--bufBitsLength))&1;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		if (!streamStatus) return 0;
		uint32_t ret=0;

		while (bufBitsLength<count)
		{
			ret=(ret<<bufBitsLength)|(bufBitsContent&((1<<bufBitsLength)-1));
			count-=bufBitsLength;
			bufBitsLength=0;

			if (bufOffset>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=bufPtr[bufOffset++];
			bufBitsLength=8;
		}
		if (count)
		{
			ret=(ret<<count)|((bufBitsContent>>(bufBitsLength-count))&((1<<count)-1));
			bufBitsLength-=count;
		}
		return ret;
	};
	
	uint8_t *dest=rawData.data();
	size_t rawSize=rawData.size();
	size_t destOffset=0;

	HuffmanDecoder<uint32_t,0x100,0> decoder;

	for (uint32_t i=0;i<256&&streamStatus;i++)
	{
		uint32_t codeLength=readBits(4);
		if (!codeLength) continue;
		if (codeLength==15) codeLength=readBits(4)+15;
		uint32_t code=readBits(codeLength);
		decoder.insert(HuffmanCode<uint32_t>{codeLength,code,i});
	}
	if (streamStatus) streamStatus=decoder.isValid();

	uint8_t accum=0;
	while (streamStatus && destOffset!=rawSize)
	{
		uint32_t code=decoder.decode(readBit);
		if (code==0x0100)
		{
			streamStatus=false;
			break;
		}
		accum+=code;
		dest[destOffset++]=accum;
	}

	return streamStatus && destOffset==rawSize;
}

XPKDecompressor::Registry<SMPLDecompressor> SMPLDecompressor::_XPKregistration;
