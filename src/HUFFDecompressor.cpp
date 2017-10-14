/* Copyright (C) Teemu Suutari */

#include "HUFFDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool HUFFDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('HUFF');
}

std::unique_ptr<XPKDecompressor> HUFFDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<HUFFDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

HUFFDecompressor::HUFFDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || packedData.size()<6)
		throw Decompressor::InvalidFormatError();
	// version: only 0 is defined
	uint16_t ver=packedData.readBE16(0);
	if (ver) throw Decompressor::InvalidFormatError();
	// password: we do not support it...
	uint32_t pwd=packedData.readBE32(2);
	if (pwd!=0xabadcafe) throw Decompressor::InvalidFormatError();
}

HUFFDecompressor::~HUFFDecompressor()
{
	// nothing needed
}

const std::string &HUFFDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-HUFF: Huffman compressor";
	return name;
}

void HUFFDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	const uint8_t *bufPtr=_packedData.data();
	size_t packedSize=_packedData.size();
	size_t bufOffset=6;

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
		uint8_t ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
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

	HuffmanDecoder<uint32_t,256,0> decoder;
	for (uint32_t i=0;i<256;i++)
	{
		uint8_t codeBits=readByte()+1;
		if (!codeBits) continue;
		if (codeBits>32) throw Decompressor::DecompressionError();
		uint32_t code=0;
		int32_t shift=-codeBits;
		for (uint32_t j=0;j<codeBits;j+=8)
		{
			code=(code<<8)|readByte();
			shift+=8;
		}
		code=(code>>shift)&((1<<codeBits)-1);
		decoder.insert(HuffmanCode<uint32_t>{codeBits,code,i});
	}

	while (destOffset!=rawSize)
		dest[destOffset++]=decoder.decode(readBit);
}

XPKDecompressor::Registry<HUFFDecompressor> HUFFDecompressor::_XPKregistration;
