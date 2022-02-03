/* Copyright (C) Teemu Suutari */

#include "LOBDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"
#include "common/OverflowCheck.hpp"


namespace ancient::internal
{

bool LOBDecompressor::detectHeader(uint32_t hdr) noexcept
{
	return hdr==FourCC("\001LOB")||hdr==FourCC("\002LOB");
}

std::shared_ptr<Decompressor> LOBDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<LOBDecompressor>(packedData,verify);
}

LOBDecompressor::LOBDecompressor(const Buffer &packedData,bool verify) :
	_packedData(packedData)
{
	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr) || packedData.size()<12U)
		throw InvalidFormatError();

	if (packedData.read8(4U)!=6U)
		throw InvalidFormatError();
	_rawSize=packedData.readBE32(4U)&0xff'ffffU;
	if (!_rawSize || _rawSize>getMaxRawSize())
		throw InvalidFormatError();
	_packedSize=OverflowCheck::sum(packedData.readBE32(8U),12U);
	if (_packedSize>packedData.size())
		throw InvalidFormatError();
}

LOBDecompressor::~LOBDecompressor()
{
	// nothing needed
}

const std::string &LOBDecompressor::getName() const noexcept
{
	static std::string name="LOB: LOB's File Compressor";
	return name;
}

size_t LOBDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t LOBDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void LOBDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	ForwardInputStream inputStream(_packedData,12U,_packedSize);
	MSBBitReader<ForwardInputStream> bitReader(inputStream);
	auto readBit=[&]()->uint32_t
	{
		return bitReader.readBits8(1U);
	};
	auto readByte=[&]()->uint8_t
	{
		return inputStream.readByte();
	};

	ForwardOutputStream outputStream(rawData,0,_rawSize);

	while (!outputStream.eof())
	{
		if (readBit())
		{
			outputStream.writeByte(readByte());
		} else {
			uint8_t byte1=readByte();
			uint8_t byte2=readByte();
			uint32_t distance=(uint32_t(byte1&0xf0)<<4)|byte2;
			uint32_t count=uint32_t(byte1&0xf)+3;

			outputStream.copy(distance,count);
		}
	}
}

}
