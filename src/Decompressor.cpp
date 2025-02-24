/* Copyright (C) Teemu Suutari */

#include "Decompressor.hpp"

#include <memory>
#include <vector>

#include "ByteKillerDecompressor.hpp"
#include "BZIP2Decompressor.hpp"
#include "CompactDecompressor.hpp"
#include "CompressDecompressor.hpp"
#include "CRMDecompressor.hpp"
#include "DEFLATEDecompressor.hpp"
#include "DMSDecompressor.hpp"
#include "FreezeDecompressor.hpp"
#include "IceDecompressor.hpp"
#include "IMPDecompressor.hpp"
#include "JAMPackerDecompressor.hpp"
#include "LOBDecompressor.hpp"
#include "MMCMPDecompressor.hpp"
#include "PackDecompressor.hpp"
#include "PMCDecompressor.hpp"
#include "PPDecompressor.hpp"
#include "RNCDecompressor.hpp"
#include "SCOCompressDecompressor.hpp"
#include "StoneCrackerDecompressor.hpp"
#include "TPWMDecompressor.hpp"
#include "VicXDecompressor.hpp"
#include "XPKMain.hpp"

namespace ancient::internal
{

// ---

static std::vector<std::pair<bool(*)(uint32_t,uint32_t),std::shared_ptr<Decompressor>(*)(const Buffer&,bool,bool)>> decompressors={
	{BZIP2Decompressor::detectHeader,BZIP2Decompressor::create},
	{CompactDecompressor::detectHeader,CompactDecompressor::create},
	{CompressDecompressor::detectHeader,CompressDecompressor::create},
	{CRMDecompressor::detectHeader,CRMDecompressor::create},
	{DEFLATEDecompressor::detectHeader,DEFLATEDecompressor::create},
	{DMSDecompressor::detectHeader,DMSDecompressor::create},
	{FreezeDecompressor::detectHeader,FreezeDecompressor::create},
	{IceDecompressor::detectHeader,IceDecompressor::create},
	{IMPDecompressor::detectHeader,IMPDecompressor::create},
	{JAMPackerDecompressor::detectHeader,JAMPackerDecompressor::create},
	{LOBDecompressor::detectHeader,LOBDecompressor::create},
	{MMCMPDecompressor::detectHeader,MMCMPDecompressor::create},
	{PackDecompressor::detectHeader,PackDecompressor::create},
	{PMCDecompressor::detectHeader,PMCDecompressor::create},
	{PPDecompressor::detectHeader,PPDecompressor::create},
	{RNCDecompressor::detectHeader,RNCDecompressor::create},
	{SCOCompressDecompressor::detectHeader,SCOCompressDecompressor::create},
	{TPWMDecompressor::detectHeader,TPWMDecompressor::create},
	{VicXDecompressor::detectHeader,VicXDecompressor::create},
	{XPKMain::detectHeader,XPKMain::create},
	// Formats with missing id / uncertain detection
	// old stonecracker is far from certain
	{StoneCrackerDecompressor::detectHeader,StoneCrackerDecompressor::create},
	{ByteKillerDecompressor::detectHeader,ByteKillerDecompressor::create}
	};

std::shared_ptr<Decompressor> Decompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	try
	{
		uint32_t hdr{(packedData.size()>=4)?packedData.readBE32(0):(uint32_t(packedData.readBE16(0))<<16)};
		uint32_t footer{(exactSizeKnown&&packedData.size()>=4)?packedData.readBE32(packedData.size()-4):0};
		for (auto &it : decompressors)
		{
			try
			{
				if (it.first(hdr,footer)) return it.second(packedData,exactSizeKnown,verify);
			} catch (const Error&) {
				// try next on the list
			}
		}
		throw InvalidFormatError();
	} catch (const Buffer::Error&) {
		throw InvalidFormatError();
	}
}

bool Decompressor::detect(const Buffer &packedData,bool exactSizeKnown) noexcept
{
	if (packedData.size()<2) return false;
	// need to create the decompressor in order to work with bad detectors.
	try
	{
		return bool(create(packedData,exactSizeKnown,true));
	} catch (const Error&) {
		return false;
	}
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

// 128M should be enough for everyone (this is retro!)
size_t Decompressor::getMaxPackedSize() noexcept
{
	return 0x800'0000U;
}

size_t Decompressor::getMaxRawSize() noexcept
{
	return 0x800'0000U;
}

}
