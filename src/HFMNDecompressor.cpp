/* Copyright (C) Teemu Suutari */

#include "HFMNDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool HFMNDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('HFMN');
}

std::unique_ptr<XPKDecompressor> HFMNDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<HFMNDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

HFMNDecompressor::HFMNDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || packedData.size()<4)
		throw Decompressor::InvalidFormatError();
	uint16_t tmp=packedData.readBE16(0);
	if (tmp&3) throw Decompressor::InvalidFormatError();	// header is being written in 4 byte chunks
	_headerSize=size_t(tmp&0x1ff);				// the top 7 bits are flags. No definition what they are and they are ignored in decoder...
	if (_headerSize+4>packedData.size()) throw Decompressor::InvalidFormatError();
	tmp=packedData.readBE16(_headerSize+2);
	_rawSize=size_t(tmp);
	if (!_rawSize) throw Decompressor::InvalidFormatError();
	_headerSize+=4;
}

HFMNDecompressor::~HFMNDecompressor()
{
	// nothing needed
}

const std::string &HFMNDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-HFMN: Huffman compressor";
	return name;
}

void HFMNDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	if (rawData.size()!=_rawSize) throw Decompressor::DecompressionError();

	// Stream reading
	const uint8_t *bufPtr=_packedData.data();
	size_t packedSize=_packedData.size();
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
		uint8_t ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	HuffmanDecoder<uint32_t,256,0> decoder;
	uint32_t code=1;
	uint32_t codeBits=1;
	for (;;)
	{
		if (!readBit())
		{
			uint32_t lit=0;
			for (uint32_t i=0;i<8;i++) lit|=readBit()<<i;
			decoder.insert(HuffmanCode<uint32_t>{codeBits,code,lit});
			while (!(code&1) && codeBits)
			{
				codeBits--;
				code>>=1;
			}
			if (!codeBits) break;	
			code--;
		} else {
			code=(code<<1)+1;
			codeBits++;
		}
	}
	if (bufOffset+2>_headerSize) throw Decompressor::DecompressionError();

	bufOffset=_headerSize;
	bufBitsContent=0;
	bufBitsLength=0;

	while (destOffset!=_rawSize)
		dest[destOffset++]=decoder.decode(readBit);
}

XPKDecompressor::Registry<HFMNDecompressor> HFMNDecompressor::_XPKregistration;
