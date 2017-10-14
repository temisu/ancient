/* Copyright (C) Teemu Suutari */

#include "SMPLDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool SMPLDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('SMPL');
}

std::unique_ptr<XPKDecompressor> SMPLDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<SMPLDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

SMPLDecompressor::SMPLDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || packedData.size()<2) throw Decompressor::InvalidFormatError();

	if (packedData.readBE16(0)!=1) throw Decompressor::InvalidFormatError();
}

SMPLDecompressor::~SMPLDecompressor()
{
	// nothing needed
}

const std::string &SMPLDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-SMPL: Huffman compressor with delta encoding";
	return name;
}

void SMPLDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();

	size_t bufOffset=2;
	uint8_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=bufPtr[bufOffset++];
			bufBitsLength=8;
		}
		return (bufBitsContent>>(--bufBitsLength))&1;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		uint32_t ret=0;

		while (bufBitsLength<count)
		{
			ret=(ret<<bufBitsLength)|(bufBitsContent&((1<<bufBitsLength)-1));
			count-=bufBitsLength;
			bufBitsLength=0;

			if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
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

	for (uint32_t i=0;i<256;i++)
	{
		uint32_t codeLength=readBits(4);
		if (!codeLength) continue;
		if (codeLength==15) codeLength=readBits(4)+15;
		uint32_t code=readBits(codeLength);
		decoder.insert(HuffmanCode<uint32_t>{codeLength,code,i});
	}

	uint8_t accum=0;
	while (destOffset!=rawSize)
	{
		uint32_t code=decoder.decode(readBit);
		accum+=code;
		dest[destOffset++]=accum;
	}
}

XPKDecompressor::Registry<SMPLDecompressor> SMPLDecompressor::_XPKregistration;
