/* Copyright (C) Teemu Suutari */

#include "Decompressor.hpp"

#include "CRMDecompressor.hpp"
#include "DEFLATEDecompressor.hpp"
#include "IMPDecompressor.hpp"
#include "PPDecompressor.hpp"
#include "RNCDecompressor.hpp"
#include "TPWMDecompressor.hpp"
#include "XPKMaster.hpp"

Decompressor::~Decompressor()
{
	// nothing needed
}

const std::string &Decompressor::getName() const
{
	static std::string name="<invalid>";
	return name;
}

std::unique_ptr<Decompressor> CreateDecompressor(const Buffer &packedData,bool exactSizeKnown)
{
	uint32_t hdr;
	if (!packedData.readBE(0,hdr)) return std::unique_ptr<Decompressor>();

	if (CRMDecompressor::detectHeader(hdr))
		return std::make_unique<CRMDecompressor>(packedData);
	if (DEFLATEDecompressor::detectHeader(hdr))
		return std::make_unique<DEFLATEDecompressor>(packedData,exactSizeKnown);
	if (IMPDecompressor::detectHeader(hdr))
		return std::make_unique<IMPDecompressor>(packedData);
	if (PPDecompressor::detectHeader(hdr))
		return std::make_unique<PPDecompressor>(packedData,exactSizeKnown);
	if (RNCDecompressor::detectHeader(hdr))
		return std::make_unique<RNCDecompressor>(packedData);
	if (TPWMDecompressor::detectHeader(hdr))
		return std::make_unique<TPWMDecompressor>(packedData);
	if (XPKMaster::detectHeader(hdr))
		return std::make_unique<XPKMaster>(packedData);

	return nullptr;
}
