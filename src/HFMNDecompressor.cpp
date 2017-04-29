/* Copyright (C) Teemu Suutari */

#include "HFMNDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool HFMNDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('HFMN');
}

bool HFMNDecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> HFMNDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<HFMNDecompressor>(hdr,packedData,state);
}

HFMNDecompressor::HFMNDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (packedData.size()<4) return;
	uint16_t tmp;
	if (!packedData.readBE(0,tmp)) return;
	if (tmp&3) return;			// header is being written in 4 byte chunks
	_headerSize=size_t(tmp&0x1ff);		// the top 7 bits are flags. No definition what they are and they are ignored in decoder...
	if (_headerSize+4>packedData.size()) return;
	if (!packedData.readBE(_headerSize+2,tmp)) return;
	_rawSize=size_t(tmp);
	if (!_rawSize) return;
	if (_rawSize>Decompressor::getMaxRawSize()) return;
	_headerSize+=4;
	_isValid=true;
}

HFMNDecompressor::~HFMNDecompressor()
{
	// nothing needed
}

bool HFMNDecompressor::isValid() const
{
	return _isValid;
}

bool HFMNDecompressor::verifyPacked() const
{
	return _isValid;
}

bool HFMNDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

const std::string &HFMNDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-HFMN: Huffman compressor";
	return name;
}

bool HFMNDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid || rawData.size()!=_rawSize) return false;

	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t packedSize=_packedData.size();
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
	while (streamStatus)
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
	if (bufOffset+2>_headerSize) streamStatus=false;
	if (streamStatus) streamStatus=decoder.isValid();

	bufOffset=_headerSize;
	bufBitsContent=0;
	bufBitsLength=0;

	while (streamStatus && destOffset<_rawSize)
	{
		uint32_t ch=decoder.decode(readBit);
		if (ch==256)
		{
			streamStatus=false;
		} else {
			dest[destOffset++]=ch;
		}
	}

	return streamStatus && destOffset==_rawSize;
}
