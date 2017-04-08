/* Copyright (C) Teemu Suutari */

#include <string.h>
#include <memory>
#include <algorithm>

#include <SubBuffer.hpp>

#include "XPKMaster.hpp"
#include "XPKDecompressor.hpp"

// Sub-decompressors
#include "ACCADecompressor.hpp"
#include "CBR0Decompressor.hpp"
#include "CRMDecompressor.hpp"
#include "DEFLATEDecompressor.hpp"
#include "DLTADecode.hpp"
#include "FASTDecompressor.hpp"
#include "FRLEDecompressor.hpp"
#include "HFMNDecompressor.hpp"
#include "HUFFDecompressor.hpp"
#include "IMPDecompressor.hpp"
#include "MASHDecompressor.hpp"
#include "NONEDecompressor.hpp"
#include "NUKEDecompressor.hpp"
#include "PPDecompressor.hpp"
#include "RAKEDecompressor.hpp"
#include "RDCNDecompressor.hpp"
#include "RLENDecompressor.hpp"
#include "SHR3Decompressor.hpp"
#include "SHRIDecompressor.hpp"
#include "SMPLDecompressor.hpp"
#include "SQSHDecompressor.hpp"
#include "TDCSDecompressor.hpp"

bool XPKMaster::detectHeader(uint32_t hdr)
{
	return hdr==FourCC('XPKF');
}

XPKMaster::XPKMaster(const Buffer &packedData) :
	_packedData(packedData)
{
	if (packedData.size()<44) return;
	uint32_t hdr;
	if (!packedData.readBE(0,hdr)) return;
	if (!detectHeader(hdr)) return;

	if (!packedData.readBE(4,_packedSize)) return;
	if (!packedData.readBE(8,_type)) return;
	if (!packedData.readBE(12,_rawSize)) return;

	if (!_rawSize || !_packedSize) return;
	if (_rawSize>getMaxRawSize() || _packedSize>getMaxPackedSize()) return;

	uint8_t flags;
	if (!packedData.read(32,flags)) return;
	_longHeaders=(flags&1)?true:false;
	if (flags&2) return;	// needs password. we do not support that
	if (flags&4)		// extra header
	{
		uint16_t extraLen;
		if (!packedData.readBE(36,extraLen)) return;
		_headerSize=38+extraLen;
	} else {
		_headerSize=36;
	}

	if (_packedSize+8>packedData.size()) return;
	_isValid=detectSubDecompressor();
}

XPKMaster::~XPKMaster()
{
	// nothing needed
}

bool XPKMaster::isValid() const
{
	return _isValid;
}

static bool headerChecksum(const Buffer &buffer,size_t offset,size_t len)
{
	if (!len || offset+len>buffer.size()) return false;
	const uint8_t *ptr=buffer.data()+offset;
	uint8_t tmp=0;
	for (size_t i=0;i<len;i++)
		tmp^=ptr[i];
	return !tmp;
}

// this implementation assumes align padding is zeros
static bool chunkChecksum(const Buffer &buffer,size_t offset,size_t len,uint16_t checkValue)
{
	if (!len || offset+len>buffer.size()) return false;
	const uint8_t *ptr=buffer.data()+offset;
	uint8_t tmp[2]={0,0};
	for (size_t i=0;i<len;i++)
		tmp[i&1]^=ptr[i];
	return tmp[0]==(checkValue>>8) && tmp[1]==(checkValue&0xff);
}

bool XPKMaster::verifyPacked() const
{
	if (!_isValid) return false;

	if (!headerChecksum(_packedData,0,36)) return false;

	std::unique_ptr<XPKDecompressor::State> state;
	return forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (!headerChecksum(header,0,header.size())) return false;

		uint16_t hdrCheck;
		if (!header.readBE(2,hdrCheck)) return false;
		if (chunk.size() && !chunkChecksum(chunk,0,chunk.size(),hdrCheck)) return false;

		if (chunkType==1)
		{
			auto sub{createSubDecompressor(chunk,state)};
			if (!sub || !sub->isValid() || !sub->verifyPacked()) return false;
		} else if (chunkType!=0 && chunkType!=15) return false;
		return true;
	});
}

// This is as good as verification of the sub-compressor...
bool XPKMaster::verifyRaw(const Buffer &rawData) const
{
	if (!_isValid || rawData.size()<_rawSize) return false;

	if (::memcmp(_packedData.data()+16,rawData.data(),std::min(_rawSize,16U))) return false;

	uint32_t destOffset=0;
	std::unique_ptr<XPKDecompressor::State> state;
	if (!forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (destOffset+rawChunkSize>rawData.size()) return false;
		if (!rawChunkSize) return true;

		ConstSubBuffer VerifyBuffer(rawData,destOffset,rawChunkSize);
		if (chunkType==1)
		{
			auto sub{createSubDecompressor(chunk,state)};
			if (!sub || !sub->isValid() || !sub->verifyRaw(VerifyBuffer)) return false;
		} else if (chunkType!=0 && chunkType!=15) return false;

		destOffset+=rawChunkSize;
		return true;
	})) return false;

	return destOffset==_rawSize;
}

const std::string &XPKMaster::getName() const
{
	if (!_isValid) return Decompressor::getName();
	std::unique_ptr<XPKDecompressor> sub;
	std::unique_ptr<XPKDecompressor::State> state;
	forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		sub=createSubDecompressor(chunk,state);
		return false;
	});
	if (sub) return sub->getSubName();
		else return Decompressor::getName();
}

size_t XPKMaster::getPackedSize() const
{
	if (!_isValid) return 0;
	return _packedSize+8;
}

size_t XPKMaster::getRawSize() const
{
	if (!_isValid) return 0;
	return _rawSize;
}

bool XPKMaster::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_rawSize) return false;

	uint32_t destOffset=0;
	std::unique_ptr<XPKDecompressor::State> state;
	if (!forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (destOffset+rawChunkSize>rawData.size()) return false;
		if (!rawChunkSize) return true;

		ConstSubBuffer previousBuffer(rawData,0,destOffset);
		SubBuffer DestBuffer(rawData,destOffset,rawChunkSize);
		switch (chunkType)
		{
			case 0:
			if (rawChunkSize!=chunk.size()) return false;
			::memcpy(DestBuffer.data(),chunk.data(),rawChunkSize);
			break;

			case 1:
			{
				auto sub{createSubDecompressor(chunk,state)};
				if (!sub || !sub->isValid() || !sub->decompress(DestBuffer,previousBuffer)) return false;
			}
			break;

			case 15:
			break;
			
			default:
			return false;
		}

		destOffset+=rawChunkSize;
		return true;
	})) return false;

	return destOffset==_rawSize;
}

bool XPKMaster::detectSubDecompressor() const
{
	if (ACCADecompressor::detectHeaderXPK(_type))
		return true;
	if (CBR0Decompressor::detectHeaderXPK(_type))
		return true;
	if (CRMDecompressor::detectHeaderXPK(_type))
		return true;
	if (DEFLATEDecompressor::detectHeaderXPK(_type))
		return true;
	if (DLTADecode::detectHeaderXPK(_type))
		return true;
	if (FASTDecompressor::detectHeaderXPK(_type))
		return true;
	if (FRLEDecompressor::detectHeaderXPK(_type))
		return true;
	if (HFMNDecompressor::detectHeaderXPK(_type))
		return true;
	if (HUFFDecompressor::detectHeaderXPK(_type))
		return true;
	if (IMPDecompressor::detectHeaderXPK(_type))
		return true;
	if (MASHDecompressor::detectHeaderXPK(_type))
		return true;
	if (NONEDecompressor::detectHeaderXPK(_type))
		return true;
	if (NUKEDecompressor::detectHeaderXPK(_type))
		return true;
	if (PPDecompressor::detectHeaderXPK(_type))
		return true;
	if (RAKEDecompressor::detectHeaderXPK(_type))
		return true;
	if (RDCNDecompressor::detectHeaderXPK(_type))
		return true;
	if (RLENDecompressor::detectHeaderXPK(_type))
		return true;
	if (SHR3Decompressor::detectHeaderXPK(_type))
		return true;
	if (SHRIDecompressor::detectHeaderXPK(_type))
		return true;
	if (SMPLDecompressor::detectHeaderXPK(_type))
		return true;
	if (SQSHDecompressor::detectHeaderXPK(_type))
		return true;
	if (TDCSDecompressor::detectHeaderXPK(_type))
		return true;
	return false;
}

std::unique_ptr<XPKDecompressor> XPKMaster::createSubDecompressor(const Buffer &buffer,std::unique_ptr<XPKDecompressor::State> &state) const
{
	if (ACCADecompressor::detectHeaderXPK(_type))
		return std::make_unique<ACCADecompressor>(_type,buffer,state);
	if (CBR0Decompressor::detectHeaderXPK(_type))
		return std::make_unique<CBR0Decompressor>(_type,buffer,state);
	if (CRMDecompressor::detectHeaderXPK(_type))
		return std::make_unique<CRMDecompressor>(_type,buffer,state);
	if (DEFLATEDecompressor::detectHeaderXPK(_type))
		return std::make_unique<DEFLATEDecompressor>(_type,buffer,state);
	if (DLTADecode::detectHeaderXPK(_type))
		return std::make_unique<DLTADecode>(_type,buffer,state);
	if (FASTDecompressor::detectHeaderXPK(_type))
		return std::make_unique<FASTDecompressor>(_type,buffer,state);
	if (FRLEDecompressor::detectHeaderXPK(_type))
		return std::make_unique<FRLEDecompressor>(_type,buffer,state);
	if (HFMNDecompressor::detectHeaderXPK(_type))
		return std::make_unique<HFMNDecompressor>(_type,buffer,state);
	if (HUFFDecompressor::detectHeaderXPK(_type))
		return std::make_unique<HUFFDecompressor>(_type,buffer,state);
	if (IMPDecompressor::detectHeaderXPK(_type))
		return std::make_unique<IMPDecompressor>(_type,buffer,state);
	if (MASHDecompressor::detectHeaderXPK(_type))
		return std::make_unique<MASHDecompressor>(_type,buffer,state);
	if (NONEDecompressor::detectHeaderXPK(_type))
		return std::make_unique<NONEDecompressor>(_type,buffer,state);
	if (NUKEDecompressor::detectHeaderXPK(_type))
		return std::make_unique<NUKEDecompressor>(_type,buffer,state);
	if (PPDecompressor::detectHeaderXPK(_type))
		return std::make_unique<PPDecompressor>(_type,buffer,state);
	if (RAKEDecompressor::detectHeaderXPK(_type))
		return std::make_unique<RAKEDecompressor>(_type,buffer,state);
	if (RDCNDecompressor::detectHeaderXPK(_type))
		return std::make_unique<RDCNDecompressor>(_type,buffer,state);
	if (RLENDecompressor::detectHeaderXPK(_type))
		return std::make_unique<RLENDecompressor>(_type,buffer,state);
	if (SHR3Decompressor::detectHeaderXPK(_type))
		return std::make_unique<SHR3Decompressor>(_type,buffer,state);
	if (SHRIDecompressor::detectHeaderXPK(_type))
		return std::make_unique<SHRIDecompressor>(_type,buffer,state);
	if (SMPLDecompressor::detectHeaderXPK(_type))
		return std::make_unique<SMPLDecompressor>(_type,buffer,state);
	if (SQSHDecompressor::detectHeaderXPK(_type))
		return std::make_unique<SQSHDecompressor>(_type,buffer,state);
	if (TDCSDecompressor::detectHeaderXPK(_type))
		return std::make_unique<TDCSDecompressor>(_type,buffer,state);
	return nullptr;
}

template <typename F>
bool XPKMaster::forEachChunk(F func) const
{
	uint32_t currentOffset=0,rawSize,packedSize;
	bool isLast=false;

	while (currentOffset<_packedSize+8 && !isLast)
	{
		auto readDualValue=[&](uint32_t offsetShort,uint32_t offsetLong,uint32_t &value)->bool
		{
			if (_longHeaders)
			{
				if (!_packedData.readBE(currentOffset+offsetLong,value)) return false;
			} else {
				uint16_t tmp;
				if (!_packedData.readBE(currentOffset+offsetShort,tmp)) return false;
				value=uint32_t(tmp);
			}
			return true;
		};

		uint32_t chunkHeaderLen=_longHeaders?12:8;
		if (!currentOffset)
		{
			// return first;
			currentOffset=_headerSize;
		} else {
			uint32_t tmp;
			if (!readDualValue(4,4,tmp)) return false;
			currentOffset+=chunkHeaderLen+((tmp+3)&~3U);
		}
		if (!readDualValue(4,4,packedSize)) return false;
		if (!readDualValue(6,8,rawSize)) return false;
		
		ConstSubBuffer hdr(_packedData,currentOffset,chunkHeaderLen);
		ConstSubBuffer chunk(_packedData,currentOffset+chunkHeaderLen,packedSize);

		uint8_t type;
		if (!_packedData.read(currentOffset,type)) return false;
		if (!func(hdr,chunk,rawSize,type)) return false;
		
		if (type==15) isLast=true;
	}
	return isLast;
}
