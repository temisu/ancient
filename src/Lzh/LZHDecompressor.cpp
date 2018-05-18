/* Copyright (C) Teemu Suutari */

#include <stdint.h>
#include <string.h>

#include <map>

#include "LZHDecompressor.hpp"

#include "LH1Decompressor.hpp"
#include "LH2Decompressor.hpp"

LZHDecompressor::LZHDecompressor(const Buffer &packedData,const std::string &method) :
	_packedData(packedData),
	_method(method)
{
	// nothing needed
}

LZHDecompressor::~LZHDecompressor()
{
	// nothing needed
}

size_t LZHDecompressor::getRawSize() const noexcept
{
	// N/A
	return 0;
}

size_t LZHDecompressor::getPackedSize() const noexcept
{
	// N/A
	return 0;
}

const std::string &LZHDecompressor::getName() const noexcept
{
	static std::string name="Lzh";
	return name;
}

void LZHDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	enum class Compressor
	{
		LH0=0,
		LH1,
		LH2,
		LZ4,
		PM0
	};

	static std::map<std::string,Compressor> compressorMap{
		{"-lh0-",Compressor::LH0},
		{"-lh1-",Compressor::LH1},
		{"-lh2-",Compressor::LH2},
		{"-lz4-",Compressor::LZ4},
		{"-pm0-",Compressor::PM0}
	};

	auto it=compressorMap.find(_method);
	if (it==compressorMap.end()) throw DecompressionError();
	switch (it->second)
	{
		case Compressor::LH0:
		case Compressor::LZ4:
		case Compressor::PM0:
		if (rawData.size()!=_packedData.size()) throw DecompressionError();
		::memcpy(rawData.data(),_packedData.data(),rawData.size());
		break;

		case Compressor::LH1:
		{
			LH1Decompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;

		case Compressor::LH2:
		{
			LH2Decompressor dec(_packedData);
			dec.decompress(rawData,verify);
		}
		break;
	}
}
