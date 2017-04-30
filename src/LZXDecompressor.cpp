/* Copyright (C) Teemu Suutari */

#include <stdint.h>
#include <string.h>

#include "LZXDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include "DLTADecode.hpp"
#include <ArrayBuffer.hpp>
#include <CRC32.hpp>

bool LZXDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('ELZX') || hdr==FourCC('SLZX');
}

bool LZXDecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> LZXDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<LZXDecompressor>(hdr,packedData,state);
}

LZXDecompressor::LZXDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (hdr==FourCC('SLZX')) _isSampled=true;
	// There is no good spec on the LZX header content -> lots of unknowns here
	if (_packedData.size()<41) return;
	{
		// XPK LZX compression is embedded single file of LZX -> read first file. Ignore rest
		uint32_t streamHdr;
		// this will include flags, which need to be zero anyway
		if (!_packedData.readBE(0,streamHdr) || streamHdr!=FourCC('LZX\0')) return;
	}
	// file header
	{
		uint32_t tmp;
		if (!_packedData.readLE(12,tmp)) return;
		_rawSize=tmp;
		if (!_packedData.readLE(16,tmp)) return;
		_packedSize=tmp;
	}
	if (!_packedData.readLE(32,_rawCRC)) return;
	uint32_t headerCRC;
	if (!_packedData.readLE(36,headerCRC)) return;
	{
		uint8_t tmp;
		if (!_packedData.read(21,tmp)) return;
		if (tmp && tmp!=2) return;
		if (tmp==2) _isCompressed=true;

		if (!_packedData.read(40,tmp)) return;
		_packedOffset=41+size_t(tmp);
		if (!_packedData.read(24,tmp)) return;
		_packedOffset+=size_t(tmp);
		_packedSize+=_packedOffset;
	}
	if (_packedSize>_packedData.size()) return;
	uint32_t crc=0;
	if (!CRC32(_packedData,10,26,crc)) return;
	ArrayBuffer<4> emptyBuf;
	for (uint32_t i=0;i<4;i++) emptyBuf[i]=0;
	if (!CRC32(emptyBuf,0,4,crc)) return;
	if (!CRC32(_packedData,40,_packedOffset-40,crc) || crc!=headerCRC) return;
	_isValid=true;
}

LZXDecompressor::~LZXDecompressor()
{
	// nothing needed
}

bool LZXDecompressor::isValid() const
{
	return _isValid;
}

bool LZXDecompressor::verifyPacked() const
{
	// header CRC already checked
	return _isValid;
}

bool LZXDecompressor::verifyRaw(const Buffer &rawData) const
{
	if (!_isValid || rawData.size()!=_rawSize) return false;
	if (_isSampled)
	{
		// Correct place for CRC is just before delta decoding
		// but that would mess the decompression flow.
		// fortunately delta decoding is trivially reversible
		uint32_t crc=0;
		uint8_t ch=0;
		const uint8_t *buffer=rawData.data();
		for (size_t i=0;i<_rawSize;i++)
		{
			uint8_t tmp=buffer[i];
			CRC32Byte(tmp-ch,crc);
			ch=tmp;
		}
		return crc==_rawCRC;
	} else {
		uint32_t crc=0;
		return CRC32(rawData,0,_rawSize,crc) && crc==_rawCRC;
	}
}

const std::string &LZXDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string nameE="XPK-ELZX: LZX-compressor";
	static std::string nameS="XPK-SLZX: LZX-compressor with delta encoding";
	return (_isSampled)?nameS:nameE;
}

bool LZXDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid || rawData.size()!=_rawSize) return false;
	if (!_isCompressed)
	{
		if (_packedSize!=_rawSize) return false;
		::memcpy(rawData.data(),_packedData.data()+_packedOffset,_rawSize);
		return true;
	}

	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_packedOffset;

	uint8_t bufBitsLength=0;
	uint32_t bufBitsContent=0;

	// streamreader
	auto readBit=[&]()->uint8_t
	{
		uint8_t ret=0;
		if (!bufBitsLength)
		{
			if (bufOffset+1>=_packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=uint32_t(bufPtr[bufOffset++])<<8;
			bufBitsContent|=uint32_t(bufPtr[bufOffset++]);
			bufBitsLength=16;
		}
		ret=bufBitsContent&1;
		bufBitsContent>>=1;
		bufBitsLength--;
		return ret;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		if (!streamStatus) return 0;
		while (bufBitsLength<count)
		{
			if (bufOffset+1>=_packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<(bufBitsLength+8);
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<bufBitsLength;
			bufBitsLength+=16;
		}
		uint32_t ret=bufBitsContent&((1<<count)-1);
		bufBitsContent>>=count;
		bufBitsLength-=count;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	typedef HuffmanDecoder<int32_t,-1,0> LZXDecoder;

	// possibly padded/reused later if multiple blocks
	uint8_t literalTable[768];
	for (uint32_t i=0;i<768;i++) literalTable[i]=0;
	LZXDecoder literalDecoder;
	uint32_t previousDistance=1;

	while (streamStatus && destOffset!=_rawSize)
	{

		auto createHuffmanTable=[&](LZXDecoder &dec,const uint8_t *bitLengths,uint32_t bitTableLength)
		{
			uint8_t minDepth=16,maxDepth=0;
			for (uint32_t i=0;i<bitTableLength;i++)
			{
				if (bitLengths[i] && bitLengths[i]<minDepth) minDepth=bitLengths[i];
				if (bitLengths[i]>maxDepth) maxDepth=bitLengths[i];
			}
			if (!maxDepth) return;

			uint32_t code=0;
			for (uint32_t depth=minDepth;depth<=maxDepth;depth++)
			{
				for (uint32_t i=0;i<bitTableLength;i++)
				{
					if (bitLengths[i]==depth)
					{
						dec.insert(HuffmanCode<int32_t>{depth,code>>(maxDepth-depth),int32_t(i)});
						code+=1<<(maxDepth-depth);
					}
				}
			}
			if (streamStatus) streamStatus=dec.isValid();
		};

		uint32_t method=readBits(3);
		if (method<1 || method>3)
			streamStatus=false;

		LZXDecoder distanceDecoder;
		if (method==3)
		{
			uint8_t bitLengths[8];
			for (uint32_t i=0;i<8;i++) bitLengths[i]=readBits(3);
			createHuffmanTable(distanceDecoder,bitLengths,8);
		}

		size_t blockLength=readBits(8)<<16;
		blockLength|=readBits(8)<<8;
		blockLength|=readBits(8);
		if (blockLength+destOffset>_rawSize)
			streamStatus=false;

		if (method!=1)
		{
			literalDecoder.reset();
			for (uint32_t pos=0,block=0;block<2&&streamStatus;block++)
			{
				uint32_t adjust=(block)?0:1;
				uint32_t maxPos=(block)?768:256;
				LZXDecoder bitLengthDecoder;
				{
					uint8_t lengthTable[20];
					for (uint32_t i=0;i<20;i++) lengthTable[i]=readBits(4);
					createHuffmanTable(bitLengthDecoder,lengthTable,20);
				}
				while (streamStatus && pos<maxPos)
				{
					int32_t symbol=bitLengthDecoder.decode(readBit);

					auto doRepeat=[&](uint32_t count,uint8_t value)
					{
						if (count>maxPos-pos) count=maxPos-pos;
						while (count--) literalTable[pos++]=value;
					};
					
					auto symDecode=[&](int32_t value)->uint32_t
					{
						if (value<0)
						{
							streamStatus=false;
							return 0;
						}
						return (literalTable[pos]+17-value)%17;
					};

					switch (symbol)
					{
						case 17:
						doRepeat(readBits(4)+3+adjust,0);
						break;

						case 18:
						doRepeat(readBits(6-adjust)+19+adjust,0);
						break;

						case 19:
						{
							uint32_t count=readBit()+3+adjust;
							doRepeat(count,symDecode(bitLengthDecoder.decode(readBit)));
						}
						break;

						default:
						literalTable[pos++]=symDecode(symbol);
						break;
					}
				}
			}
			createHuffmanTable(literalDecoder,literalTable,768);
		}
		
		while (streamStatus && blockLength)
		{
			int32_t symbol=literalDecoder.decode(readBit);
			if (symbol<0)
			{
				streamStatus=false;
			} else if (symbol<256) {
				if (destOffset>=_rawSize)
				{
					streamStatus=false;
					break;
				}
				dest[destOffset++]=symbol;
				blockLength--;
			} else {
				// both of these tables are almost too regular to be tables...
				static const uint8_t ldBits[32]={
					0,0,0,0,1,1,2,2,
					3,3,4,4,5,5,6,6,
					7,7,8,8,9,9,10,10,
					11,11,12,12,13,13,14,14};
				static const uint32_t ldAdditions[32]={
					   0x0,
					   0x1,   0x2,   0x3,   0x4,   0x6,   0x8,   0xc, 0x10,
					  0x18,  0x20,  0x30,  0x40,  0x60,  0x80,  0xc0, 0x100,
					 0x180, 0x200, 0x300, 0x400, 0x600, 0x800, 0xc00,0x1000,
					0x1800,0x2000,0x3000,0x4000,0x6000,0x8000,0xc000};

				symbol-=256;
				uint32_t bits=ldBits[symbol&0x1f];
				uint32_t distance=ldAdditions[symbol&0x1f];
				if (bits>=3 && method==3)
				{
					distance+=readBits(bits-3)<<3;
					int32_t tmp=distanceDecoder.decode(readBit);
					if (tmp<0)
					{
						streamStatus=false;
					} else {
						distance+=tmp;
					}
				} else {
					distance+=readBits(bits);
					if (!distance) distance=previousDistance;
				}
				previousDistance=distance;

				uint32_t count=ldAdditions[symbol>>5]+readBits(ldBits[symbol>>5])+3;
				if (distance>destOffset || count>blockLength || destOffset+count>_rawSize)
				{
					streamStatus=false;
				} else {
					for (uint32_t i=0;i<count;i++,destOffset++,blockLength--)
						dest[destOffset]=dest[destOffset-distance];
				}
			}
		}
	}
	bool ret=(streamStatus && _rawSize==destOffset);
	if (ret && _isSampled)
		DLTADecode::decode(rawData,rawData,0,_rawSize);
	return ret;
}
