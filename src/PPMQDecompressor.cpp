/* Copyright (C) Teemu Suutari */

#include "PPMQDecompressor.hpp"
#include "RangeDecoder.hpp"
#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "common/Common.hpp"


namespace ancient::internal
{

bool PPMQDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC("PPMQ");
}

std::shared_ptr<XPKDecompressor> PPMQDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_shared<PPMQDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

PPMQDecompressor::PPMQDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::shared_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	if (packedData.size()<2) throw Decompressor::InvalidFormatError(); 
}

PPMQDecompressor::~PPMQDecompressor()
{
	// nothing needed
}

const std::string &PPMQDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-PPMQ: compressor";
	return name;
}

void PPMQDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
}

}
