/* Copyright (C) Teemu Suutari */

#include "Decompressor.hpp"

namespace ancient
{

std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<Decompressor>(*)(const Buffer&,bool,bool)>> *Decompressor::_decompressors=nullptr;

Decompressor::~Decompressor()
{
	// nothing needed
}

std::unique_ptr<Decompressor> Decompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	try
	{
		uint32_t hdr=packedData.readBE32(0);
		for (auto &it : *_decompressors)
		{
			if (it.first(hdr)) return it.second(packedData,exactSizeKnown,verify);
		}
		throw InvalidFormatError();
	} catch (const Buffer::Error&) {
		throw InvalidFormatError();
	}
}

bool Decompressor::detect(const Buffer &packedData) noexcept
{
	try
	{
		uint32_t hdr=packedData.readBE32(0);
		for (auto &it : *_decompressors)
			if (it.first(hdr)) return true;
		return false;
	} catch (const Buffer::Error&) {
		return false;
	}
}

void Decompressor::registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<Decompressor>(*create)(const Buffer&,bool,bool))
{
	static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<Decompressor>(*)(const Buffer&,bool,bool)>> _list;
	if (!_decompressors) _decompressors=&_list;
	_decompressors->emplace_back(detect,create);
}

void Decompressor::decompress(Buffer &rawData,bool verify)
{
	// Simplifying the implementation of sub-decompressors. Just let the buffer-exception pass here,
	// and that will get translated into Decompressor exceptions
	try
	{
		decompressImpl(rawData,verify);
	} catch (const Buffer::Error&) {
		throw DecompressionError();
	}
}

size_t Decompressor::getImageSize() const noexcept
{
	return 0;
}

size_t Decompressor::getImageOffset() const noexcept
{
	return 0;
}

}
