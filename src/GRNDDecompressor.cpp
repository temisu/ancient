/* Copyright (C) Teemu Suutari */

#include <algorithm>

#include "GRNDDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/CRC16.hpp"
#include "common/OverflowCheck.hpp"
#include "common/Common.hpp"

#include <array>

namespace ancient::internal
{

bool GRNDDecompressor::detectHeader(uint32_t hdr,uint32_t footer) noexcept
{
	return hdr==FourCC("GrnD")||hdr==FourCC("GrND");
}

std::shared_ptr<Decompressor> GRNDDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_shared<GRNDDecompressor>(packedData,verify);
}

GRNDDecompressor::GRNDDecompressor(const Buffer &packedData,bool verify) :
	_packedData{packedData}
{
	uint32_t hdr{packedData.readBE32(0)};
	if (!detectHeader(hdr,0))
		throw InvalidFormatError();
	_singleBlock=hdr==FourCC("GrND");
}

const std::string &GRNDDecompressor::getName() const noexcept
{
	static std::string names[2]={
		{"GrnD: Grind Compressor"},
		{"GrND: Grind Compressor (single block)"}};
	return names[_singleBlock?1:0];
}

size_t GRNDDecompressor::getPackedSize() const noexcept
{
	return _packedSize;
}

size_t GRNDDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void GRNDDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	ForwardInputStream inputStream{_packedData,4U,_packedData.size()};
	LSBBitReader<ForwardInputStream> bitReader{inputStream};
	bitReader.reset(inputStream.readBE32(),32U);

	auto ensureBits=[&]()
	{
		bitReader.ensureBitsGeneric(17U,[&](){return std::make_pair(inputStream.readBE16(),16U);});
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		return bitReader.readBitsBE16(count);
	};

	auto readBitsAndFill=[&](uint32_t count)->uint32_t
	{
		ensureBits();
		return bitReader.readBitsBE16(count);
	};

	auto readBit=[&]()->uint32_t
	{
		return readBits(1U);
	};

	AutoExpandingForwardOutputStream outputStream{rawData};

	uint32_t previousDistance{0};
	for (;;) // chunk loop
	{
		HuffmanDecoder<uint32_t> symbolDecoder;
		HuffmanDecoder<uint32_t> litCountDecoder;
		HuffmanDecoder<uint32_t> lengthDecoder;
		HuffmanDecoder<uint32_t> distanceDecoder;

		// helpers
		auto readHuffmanTable=[&](HuffmanDecoder<uint32_t> &dec)
		{
			uint32_t length{readBitsAndFill(4U)+1U};
			std::array<uint8_t,16> lengthTable;
			for (uint32_t i=0;i<length;i++)
			{
				uint8_t bitLength=readBitsAndFill(3U);
				if (!bitLength) bitLength=readBitsAndFill(3U);
					else bitLength++;
				lengthTable[i]=bitLength;
			}
			dec.createOrderlyHuffmanTable(lengthTable,length);
		};

		auto readTables=[&]()
		{
			{
				std::array<uint8_t,256> lengthTable;
				for (uint32_t i=0;i<256U;)
				{
					uint8_t bitLength{uint8_t(readBitsAndFill(3U))};
					if (bitLength) bitLength+=3U;
					uint32_t count;
					if (bitLength==4U)
					{
						count=readBitsAndFill(4U);
						if (count) count+=3U;
							else count=1U;
						bitLength=readBitsAndFill(3U);
						if (bitLength) bitLength+=3U;
					} else count=1U;
					if (i+count>256)
						throw DecompressionError();
					for (uint32_t j=0;j<count;j++) lengthTable[i+j]=bitLength;
					i+=count;
				}
				symbolDecoder.createOrderlyHuffmanTable(lengthTable,256U);
			}

			readHuffmanTable(litCountDecoder);
			readHuffmanTable(lengthDecoder);
			readHuffmanTable(distanceDecoder);
		};

		auto huffmanDecode=[&](const HuffmanDecoder<uint32_t> &dec)->uint32_t
		{
			uint32_t huffBits{0};
			ensureBits();
			uint32_t ret{dec.decode([&](){huffBits++;return readBit();})};
			if (ret>=2U)
			{
				uint32_t bits{ret-1U};
				ret=1U<<bits;
				if (huffBits+bits<=8U)
				{
					ret|=rotateBits(readBits(bits),bits);
				} else {
					uint32_t valueBits{8U-huffBits};
					if (valueBits)
						ret|=rotateBits(readBits(valueBits),valueBits)<<(huffBits+bits-8U);
					ret|=readBitsAndFill(huffBits+bits-8U);
				}
			}
			return ret;
		};

		uint32_t blockStart{uint32_t(inputStream.getOffset())&~511U};

		uint32_t blockDef{readBitsAndFill(10U)};
		uint32_t blocks{(blockDef+1U)&511U};
		bool blockNoPad{bool(blockDef&512U)};
		readTables();

		for (uint32_t sub=0;sub<blocks;sub++)
		{
			uint32_t blockSize{readBitsAndFill(10U)+1U};
			for (;;)
			{
				uint32_t literalCount{huffmanDecode(litCountDecoder)};
				for (uint32_t i=0;i<literalCount;i++)
				{
					ensureBits();
					outputStream.writeByte(symbolDecoder.decode(readBit));
				}
				if (!--blockSize) break;
				uint32_t count{huffmanDecode(lengthDecoder)+3U};
				uint32_t distance{huffmanDecode(distanceDecoder)};
				if (!distance) distance=previousDistance;
				previousDistance=distance;
				outputStream.copy(distance,count);
				if (!--blockSize) break;
			}
			// blocks are not suggestions here! (they are physical tracks)
			if (!_singleBlock && OverflowCheck::sum(blockStart,512U)<inputStream.getOffset())
				throw DecompressionError();
			uint32_t padding{512U-(uint32_t(inputStream.getOffset())&511U)};
			if (padding==512U || _singleBlock) padding=0;
			if (sub+1==blocks && blockNoPad) padding=0;
			if (inputStream.getOffset()+padding>=inputStream.getEndOffset())
			{
				_packedSize=uint32_t(inputStream.getOffset());
				_rawSize=uint32_t(outputStream.getOffset());
				return;
			}
			inputStream.consume(padding);

			blockStart=uint32_t(inputStream.getOffset())&~511U;
		}
	}
}

}
