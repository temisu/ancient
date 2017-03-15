/* Copyright (C) Teemu Suutari */

#include "DLTADecode.hpp"

bool DLTADecode::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('DLTA');
}

DLTADecode::DLTADecode(uint32_t hdr,const Buffer &packedData) :
	Decompressor(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	_isValid=true;
}

DLTADecode::~DLTADecode()
{
	// nothing needed
}

bool DLTADecode::isValid() const
{
	return _isValid;
}

bool DLTADecode::verifyPacked() const
{
	return _isValid;
}

bool DLTADecode::verifyRaw(const Buffer &rawData) const
{
	return _isValid;
}

const std::string &DLTADecode::getSubName() const
{
	if (!_isValid) return Decompressor::getSubName();
	static std::string name="XPK-DLTA: Delta encoding";
	return name;
}

size_t DLTADecode::getPackedSize() const
{
	return 0;
}

size_t DLTADecode::getRawSize() const
{
	if (!_isValid) return 0;
	return _packedData.size();
}

bool DLTADecode::decode(Buffer &bufferDest,const Buffer &bufferSrc,size_t offset,size_t size)
{
	if (bufferSrc.size()<offset+size) return false;
	if (bufferDest.size()<offset+size) return false;
	const uint8_t *src=bufferSrc.data()+offset;
	uint8_t *dest=bufferDest.data()+offset;

	uint8_t ctr=0;
	for (size_t i=0;i<size;i++)
	{
		ctr+=src[i];
		dest[i]=ctr;
	}
	return true;
}


bool DLTADecode::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_packedData.size()) return false;

	return decode(rawData,_packedData,0,_packedData.size());
}
