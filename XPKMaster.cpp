/* Copyright (C) Teemu Suutari */

#include <string.h>
#include <memory>
#include <algorithm>

#include "XPKMaster.hpp"

// Sub-decompressors
#include "CRMDecompressor.hpp"
#include "DEFLATEDecompressor.hpp"
#include "MASHDecompressor.hpp"
#include "NONEDecompressor.hpp"
#include "SQSHDecompressor.hpp"

bool XPKMaster::detectHeader(uint32_t hdr)
{
	return (hdr==FourCC('XPKF'));
}

XPKMaster::XPKMaster(const Buffer &packedData) :
	Decompressor(packedData)
{
	if (packedData.size()<44) return;
	uint32_t hdr;
	if (!packedData.readBE(0,hdr)) return;
	if (!detectHeader(hdr)) return;

	if (!packedData.readBE(4,_packedSize)) return;
	if (!packedData.readBE(8,_type)) return;
	if (!packedData.readBE(12,_rawSize)) return;
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
	_isValid=true;
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

	return forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (!headerChecksum(header,0,header.size())) return false;

		uint16_t hdrCheck;
		if (!header.readBE(2,hdrCheck)) return false;
		if (chunk.size() && !chunkChecksum(chunk,0,chunk.size(),hdrCheck)) return false;

		if (chunkType==1)
		{
			std::unique_ptr<Decompressor> sub{createSubDecompressor(chunk)};
			if (!sub || !sub->isValid() || (sub->getRawSize() && sub->getRawSize()!=rawChunkSize) ||
				!sub->verifyPacked()) return false;
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
	if (!forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (destOffset+rawChunkSize>rawData.size()) return false;
		if (!rawChunkSize) return true;

		ConstSubBuffer VerifyBuffer(rawData,destOffset,rawChunkSize);
		if (chunkType==1)
		{
			std::unique_ptr<Decompressor> sub{createSubDecompressor(chunk)};
			if (!sub || !sub->isValid() || (sub->getRawSize() && sub->getRawSize()!=rawChunkSize) ||
				!sub->verifyRaw(VerifyBuffer)) return false;
		} else if (chunkType!=0 && chunkType!=15) return false;

		destOffset+=rawChunkSize;
		return true;
	})) return false;

	return destOffset==_rawSize;
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
	if (!forEachChunk([&](const Buffer &header,const Buffer &chunk,uint32_t rawChunkSize,uint8_t chunkType)->bool
	{
		if (destOffset+rawChunkSize>rawData.size()) return false;
		if (!rawChunkSize) return true;

		SubBuffer DestBuffer(rawData,destOffset,rawChunkSize);
		switch (chunkType)
		{
			case 0:
			if (rawChunkSize!=chunk.size()) return false;
			::memcpy(DestBuffer.data(),chunk.data(),rawChunkSize);
			break;

			case 1:
			{
				std::unique_ptr<Decompressor> sub{createSubDecompressor(chunk)};
				if (!sub || !sub->isValid() || (sub->getRawSize() && sub->getRawSize()!=rawChunkSize) ||
					!sub->decompress(DestBuffer)) return false;
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

Decompressor *XPKMaster::createSubDecompressor(const Buffer &buffer) const
{
	if (CRMDecompressor::detectHeaderXPK(_type))
		return new CRMDecompressor(_type,buffer);
	if (DEFLATEDecompressor::detectHeaderXPK(_type))
		return new DEFLATEDecompressor(_type,buffer);
	if (MASHDecompressor::detectHeaderXPK(_type))
		return new MASHDecompressor(_type,buffer);
	if (NONEDecompressor::detectHeaderXPK(_type))
		return new NONEDecompressor(_type,buffer);
	if (SQSHDecompressor::detectHeaderXPK(_type))
		return new SQSHDecompressor(_type,buffer);
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
