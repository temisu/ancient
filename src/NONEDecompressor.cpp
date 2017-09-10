/* Copyright (C) Teemu Suutari */

#include <string.h>

#include "NONEDecompressor.hpp"

bool NONEDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('NONE');
}

std::unique_ptr<XPKDecompressor> NONEDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<NONEDecompressor>(hdr,recursionLevel,packedData,state);
}

NONEDecompressor::NONEDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
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

const std::string &NONEDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-NONE: Null compressor";
	return name;
}

bool NONEDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid || rawData.size()<_packedData.size()) return false;

	::memcpy(rawData.data(),_packedData.data(),_packedData.size());
	return true;
}

XPKDecompressor::Registry<NONEDecompressor> NONEDecompressor::_XPKregistration;
