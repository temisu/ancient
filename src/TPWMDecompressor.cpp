/* Copyright (C) Teemu Suutari */

#include "TPWMDecompressor.hpp"

bool TPWMDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC('TPWM');
}

std::unique_ptr<Decompressor> TPWMDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<TPWMDecompressor>(packedData,verify);
}

TPWMDecompressor::TPWMDecompressor(const Buffer &packedData,bool verify) :
	_packedData(packedData)
{
	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr) || packedData.size()<12) throw InvalidFormatError();

	_rawSize=packedData.readBE32(4);
	if (!_rawSize || _rawSize>getMaxRawSize())
		throw InvalidFormatError();
}

TPWMDecompressor::~TPWMDecompressor()
{
	// nothing needed
}

const std::string &TPWMDecompressor::getName() const noexcept
{
	static std::string name="TPWM: Turbo Packer";
	return name;
}

size_t TPWMDecompressor::getPackedSize() const noexcept
{
	// No packed size in the stream :(
	// After decompression, we can tell how many bytes were actually used
	return _decompressedPackedSize;
}

size_t TPWMDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void TPWMDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	// Stream reading
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=8;
	size_t packedSize=_packedData.size();
	uint8_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBit=[&]()->uint8_t
	{
		uint8_t ret=0;
		if (!bufBitsLength)
		{
			if (bufOffset>=packedSize) throw DecompressionError();
			bufBitsContent=bufPtr[bufOffset++];
			bufBitsLength=8;
		}
		ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (bufOffset>=packedSize) throw DecompressionError();
		return bufPtr[bufOffset++];
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	while (destOffset!=_rawSize)
	{
		if (readBit())
		{
			uint8_t byte1=readByte();
			uint8_t byte2=readByte();
			uint32_t distance=(uint32_t(byte1&0xf0)<<4)|byte2;
			uint32_t count=uint32_t(byte1&0xf)+3;
			if (!distance || distance>destOffset) throw DecompressionError();
			for (uint32_t i=0;i<count;i++)
			{
				dest[destOffset]=dest[destOffset-distance];
				destOffset++;
				if (destOffset==_rawSize) break;
			}
		} else {
			dest[destOffset++]=readByte();
		}
	}

	_decompressedPackedSize=bufOffset;
}

Decompressor::Registry<TPWMDecompressor> TPWMDecompressor::_registration;
