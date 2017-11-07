/* Copyright (C) Teemu Suutari */

#include "BLZWDecompressor.hpp"

bool BLZWDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('BLZW');
}

std::unique_ptr<XPKDecompressor> BLZWDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<BLZWDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

BLZWDecompressor::BLZWDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
	_maxBits=_packedData.readBE16(0);
	if (_maxBits<9 || _maxBits>20) throw Decompressor::InvalidFormatError();;
	_stackLength=uint32_t(_packedData.readBE16(2))+5;
}

BLZWDecompressor::~BLZWDecompressor()
{
	// nothing needed
}

const std::string &BLZWDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-BLZW: LZW-compressor";
	return name;
}

void BLZWDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	// Stream reading
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=4;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint32_t bits)->uint32_t
	{
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize) throw Decompressor::DecompressionError();
			bufBitsContent=(bufBitsContent<<8)|bufPtr[bufOffset++];
			bufBitsLength+=8;
		}

		uint32_t ret=(bufBitsContent>>(bufBitsLength-bits))&((1<<bits)-1);
		bufBitsLength-=bits;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;
	size_t rawSize=rawData.size();

	uint32_t maxCode=1<<_maxBits;
	auto prefix=std::make_unique<uint32_t[]>(maxCode-259);
	auto suffix=std::make_unique<uint8_t[]>(maxCode-259);
	auto stack=std::make_unique<uint8_t[]>(_stackLength);

	uint32_t freeIndex,codeBits,prevCode,newCode;

	auto suffixLookup=[&](uint32_t code)->uint32_t
	{
		if (code>=freeIndex) throw Decompressor::DecompressionError();
		return (code<259)?code:suffix[code-259];
	};

	auto insert=[&](uint32_t code)
	{
		uint32_t stackPos=0;
		newCode=suffixLookup(code);
		while (code>=259)
		{
			if (stackPos+1>=_stackLength) throw Decompressor::DecompressionError();
			stack[stackPos++]=newCode;
			code=prefix[code-259];
			newCode=suffixLookup(code);
		}
		stack[stackPos++]=newCode;
		if (destOffset+stackPos>rawSize) throw Decompressor::DecompressionError();
		while (stackPos) dest[destOffset++]=stack[--stackPos];
	};

	auto init=[&]()
	{
		codeBits=9;
		freeIndex=259;
		prevCode=readBits(codeBits);
		insert(prevCode);
	};

	init();
	while (destOffset!=rawSize)
	{
		uint32_t code=readBits(codeBits);
		bool doExit=false;
		switch (code)
		{
			case 256:
			doExit=true;
			break;

			case 257:
			init();
			break;

			case 258:
			codeBits++;
			break;

			default:
			if (code>=freeIndex)
			{
				uint32_t tmp=newCode;
				insert(prevCode);
				if (destOffset>=rawSize) throw Decompressor::DecompressionError();
				dest[destOffset++]=tmp;
			} else insert(code);
			if (freeIndex<maxCode)
			{
				suffix[freeIndex-259]=newCode;
				prefix[freeIndex-259]=prevCode;
				freeIndex++;
			}
			prevCode=code;
			break;
		}
		if (doExit) break;
	}
	if (destOffset!=rawSize) throw Decompressor::DecompressionError();
}

XPKDecompressor::Registry<BLZWDecompressor> BLZWDecompressor::_XPKregistration;
