/* Copyright (C) Teemu Suutari */

#include <string.h>

#include <SubBuffer.hpp>
#include "CYB2Decoder.hpp"
#include "XPKMaster.hpp"

bool CYB2Decoder::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('CYB2');
}

std::unique_ptr<XPKDecompressor> CYB2Decoder::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<CYB2Decoder>(hdr,recursionLevel,packedData,state);
}

CYB2Decoder::CYB2Decoder(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (_packedData.size()<=10) return;
	if (!_packedData.readBE(0,_blockHeader)) return;
	// after the block header, the next 6 bytes seem to be
	// 00 64 00 00 00 00
	// Those bytes do not seem to be terribly important though...
	_isValid=true;
}

CYB2Decoder::~CYB2Decoder()
{
	// nothing needed
}

bool CYB2Decoder::isValid() const
{
	return _isValid;
}

bool CYB2Decoder::verifyPacked() const
{
	if (!_isValid) return false;

	ConstSubBuffer blockData(_packedData,10,_packedData.size()-10);
	std::unique_ptr<XPKDecompressor::State> state;
	auto sub=XPKMaster::createDecompressor(_blockHeader,_recursionLevel+1,blockData,state);
	return sub && sub->isValid() && sub->verifyPacked();
}

bool CYB2Decoder::verifyRaw(const Buffer &rawData) const
{
	if (!_isValid) return false;

	ConstSubBuffer blockData(_packedData,10,_packedData.size()-10);
	std::unique_ptr<XPKDecompressor::State> state;
	auto sub=XPKMaster::createDecompressor(_blockHeader,_recursionLevel+1,blockData,state);
	return sub && sub->isValid() && sub->verifyRaw(rawData);
}

const std::string &CYB2Decoder::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-CYB2: xpkCybPrefs container";
	return name;
}

bool CYB2Decoder::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	ConstSubBuffer blockData(_packedData,10,_packedData.size()-10);
	std::unique_ptr<XPKDecompressor::State> state;
	auto sub=XPKMaster::createDecompressor(_blockHeader,_recursionLevel+1,blockData,state);
	return sub && sub->isValid() && sub->decompress(rawData,previousData);
}

static XPKDecompressor::Registry<CYB2Decoder> CYB2Registration;