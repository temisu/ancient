/* Copyright (C) Teemu Suutari */

#include <string.h>

#include <SubBuffer.hpp>
#include "SDHCDecompressor.hpp"
#include "XPKMaster.hpp"
#include "DLTADecode.hpp"

bool SDHCDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('SDHC');
}

std::unique_ptr<XPKDecompressor> SDHCDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<SDHCDecompressor>(hdr,recursionLevel,packedData,state);
}

SDHCDecompressor::SDHCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (_packedData.size()<2) return;
	if (!_packedData.readBE(0,_mode)) return;
	_isValid=true;
}

SDHCDecompressor::~SDHCDecompressor()
{
	// nothing needed
}

bool SDHCDecompressor::isValid() const
{
	return _isValid;
}

bool SDHCDecompressor::verifyPacked() const
{
	if (!_isValid) return false;

	if (_mode&0x8000U)
	{
		ConstSubBuffer src(_packedData,2,_packedData.size()-2);
		XPKMaster master(src,_recursionLevel+1);
		if (!master.isValid() || !master.verifyPacked()) return false;
	}
	return true;
}

bool SDHCDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done (data is modified)
	return _isValid;
}

const std::string &SDHCDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-SDHC: Sample delta huffman compressor";
	return name;
}

bool SDHCDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	ConstSubBuffer src(_packedData,2,_packedData.size()-2);
	if (_mode&0x8000U)
	{
		XPKMaster master(src,_recursionLevel+1);
		if (!master.isValid() || !master.decompress(rawData)) return false;
	} else {
		if (src.size()!=rawData.size()) return false;
		::memcpy(rawData.data(),src.data(),src.size());
	}

	size_t length=rawData.size()&~3U;

	auto deltaDecodeMono=[&]()
	{
		uint8_t *buf=rawData.data();

		uint16_t ctr=0;
		for (size_t i=0;i<length;i+=2)
		{
			uint16_t tmp;
			tmp=(uint16_t(buf[i])<<8)|uint16_t(buf[i+1]);
			ctr+=tmp;
			buf[i]=ctr>>8;
			buf[i+1]=ctr;
		}
	};

	auto deltaDecodeStereo=[&]()
	{
		uint8_t *buf=rawData.data();

		uint16_t ctr1=0,ctr2=0;
		for (size_t i=0;i<length;i+=4)
		{
			uint16_t tmp;
			tmp=(uint16_t(buf[i])<<8)|uint16_t(buf[i+1]);
			ctr1+=tmp;
			tmp=(uint16_t(buf[i+2])<<8)|uint16_t(buf[i+3]);
			ctr2+=tmp;
			buf[i]=ctr1>>8;
			buf[i+1]=ctr1;
			buf[i+2]=ctr2>>8;
			buf[i+3]=ctr2;
		}
	};

	switch (_mode&15)
	{
		case 1:
		DLTADecode::decode(rawData,rawData,0,length);
		// intentional fall through
		case 0:
		DLTADecode::decode(rawData,rawData,0,length);
		break;
		
		case 3:
		deltaDecodeMono();
		// intentional fall through
		case 2:
		deltaDecodeMono();
		break;

		case 11:
		deltaDecodeStereo();
		// intentional fall through
		case 10:
		deltaDecodeStereo();
		break;

		default:
		return false;
	}

	return true;
}

XPKDecompressor::Registry<SDHCDecompressor> SDHCDecompressor::_XPKregistration;
