/* Copyright (C) Teemu Suutari */

#include "ARTMDecompressor.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"

ARTMDecompressor::ArithDecoder::ArithDecoder(ForwardInputStream &inputStream) :
	_bitReader(inputStream)
{
	_stream=0;
	for (uint32_t i=0;i<16;i++) _stream=(_stream<<1)|_bitReader.readBits8(1);
}

ARTMDecompressor::ArithDecoder::~ArithDecoder()
{
	// nothing needed
}

uint16_t ARTMDecompressor::ArithDecoder::decode(uint16_t length)
{
	return ((uint32_t(_stream-_low)+1)*length-1)/(uint32_t(_high-_low)+1);
}

void ARTMDecompressor::ArithDecoder::scale(uint16_t newLow,uint16_t newHigh,uint16_t newRange)
{
	auto readBit=[&]()->uint32_t
	{
		return _bitReader.readBits8(1);
	};

	uint32_t range=uint32_t(_high-_low)+1;
	_high=(range*newHigh)/newRange+_low-1;
	_low=(range*newLow)/newRange+_low;

	for (;;)
	{
		auto doubleContext=[&](uint16_t decr)
		{
			_low-=decr;
			_high-=decr;
			_stream-=decr;
			_low<<=1;
			_high=(_high<<1)|1U;
			_stream=(_stream<<1)|readBit();
		};

		if (_high<0x8000U)
		{
			doubleContext(0U);
			continue;
		}
		
		if (_low>=0x8000U)
		{
			doubleContext(0x8000U);
			continue;
		}

		if (_low>=0x4000U && _high<0xC000U)
		{
			doubleContext(0x4000U);
			continue;
		}

		break;
	}
}


bool ARTMDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('ARTM');
}

std::unique_ptr<XPKDecompressor> ARTMDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<ARTMDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

ARTMDecompressor::ARTMDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	if (packedData.size()<2) throw Decompressor::InvalidFormatError(); 
}

ARTMDecompressor::~ARTMDecompressor()
{
	// nothing needed
}

const std::string &ARTMDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-ARTM: Arithmetic encoding compressor";
	return name;
}

// Apart from the peculiar ArithDecoder above there really is not much to see here.
// Except maybe for the fact that there is extra symbol defined but never used.
// It is used in the original implementation as a terminator. In here it is considered as an error.
// Logically it would decode into null-character (practically it would be instant buffer overflow)
void ARTMDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size(),true);
	ForwardOutputStream outputStream(rawData,0,rawData.size());

	ArithDecoder decoder(inputStream);

	uint8_t characters[256];
	uint16_t frequencies[256];
	uint16_t frequencySums[256];

	for (uint32_t i=0;i<256;i++)
	{
		characters[i]=i;
		frequencies[i]=1;
		frequencySums[i]=256-i;
	}
	uint16_t frequencyTotal=257;

	while (!outputStream.eof())
	{
		uint16_t value=decoder.decode(frequencyTotal);
		uint16_t symbol;
		for (symbol=0;symbol<256;symbol++)
			if (frequencySums[symbol]<=value) break;
		if (symbol==256) throw Decompressor::DecompressionError();
		decoder.scale(frequencySums[symbol],frequencySums[symbol]+frequencies[symbol],frequencyTotal);
		outputStream.writeByte(characters[symbol]);

		if (frequencyTotal==0x3fffU)
		{
			frequencyTotal=1;
			for (int32_t i=255;i>=0;i--)
			{
				frequencySums[i]=frequencyTotal;
				frequencyTotal+=frequencies[i]=(frequencies[i]+1)>>1;
			}
		}
		
		uint16_t i;
		for (i=symbol;i&&frequencies[i-1]==frequencies[i];i--);
		if (i!=symbol)
			std::swap(characters[symbol],characters[i]);
		frequencies[i]++;
		while (i--) frequencySums[i]++;
		frequencyTotal++;
	}
}

XPKDecompressor::Registry<ARTMDecompressor> ARTMDecompressor::_XPKregistration;
