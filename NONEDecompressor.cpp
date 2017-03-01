/* Copyright (C) Teemu Suutari */

#include <string.h>

#include "NONEDecompressor.hpp"

bool NONEDecompressor::detectHeader(uint32_t hdr)
{
	if (hdr==FourCC('NONE')) return true;
		else return false;
}

NONEDecompressor::NONEDecompressor(uint32_t hdr,const Buffer &packedData) :
	Decompressor(packedData),
	_isValid(false)
{
	if (!detectHeader(hdr)) return;
	_isValid=true;
}

NONEDecompressor::~NONEDecompressor()
{
	// nothing needed
}

bool NONEDecompressor::isValid() const
{
	return _isValid;
}

bool NONEDecompressor::verifyPacked() const
{
	return _isValid;
}

bool NONEDecompressor::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

size_t NONEDecompressor::getRawSize() const
{
	if (!_isValid) return 0;
	return _packedData.size();
}

bool NONEDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_packedData.size()) return false;

	::memcpy(rawData.data(),_packedData.data(),_packedData.size());
	return true;
}
