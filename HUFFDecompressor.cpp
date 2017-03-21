/* Copyright (C) Teemu Suutari */

#include "HUFFDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool HUFFDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('HUFF');
}

HUFFDecompressor::HUFFDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (packedData.size()<6) return;
	// version: only 0 is defined
	uint16_t ver;
	if (!packedData.readBE(0,ver)) return;
	if (ver) return;
	// password: we do not support it...
	uint32_t pwd;
	if (!packedData.readBE(2,pwd)) return;
	if (pwd!=0xabadcafe) return;
	_isValid=true;
}

HUFFDecompressor::~HUFFDecompressor()
{
	// nothing needed
}

bool HUFFDecompressor::isValid() const
{
	return _isValid;
}

bool HUFFDecompressor::verifyPacked() const
{
	return _isValid;
}

bool HUFFDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

const std::string &HUFFDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-HUFF: Huffman compressor";
	return name;
}

bool HUFFDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t packedSize=_packedData.size();
	size_t bufOffset=6;

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

	HuffmanDecoder<uint32_t,256,0> decoder;
	for (uint32_t i=0;i<256&&streamStatus;i++)
	{
		uint8_t codeBits=readByte()+1;
		if (!codeBits) continue;
		if (codeBits>32)
		{
			streamStatus=false;
			break;
		}
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
	if (streamStatus) streamStatus=decoder.isValid();

	while (streamStatus && destOffset<rawSize)
	{
		uint32_t ch=decoder.decode(readBit);
		if (ch==256)
		{
			streamStatus=false;
		} else {
			dest[destOffset++]=ch;
		}
	}

	return streamStatus && destOffset==rawSize;
}
