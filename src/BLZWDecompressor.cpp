/* Copyright (C) Teemu Suutari */

#include "BLZWDecompressor.hpp"
#include "HuffmanDecoder.hpp"

bool BLZWDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('BLZW');
}

std::unique_ptr<XPKDecompressor> BLZWDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<BLZWDecompressor>(hdr,packedData,state);
}

BLZWDecompressor::BLZWDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	uint16_t tmp;
	if (!_packedData.readBE(0,tmp) || tmp<9 || tmp>24) return;
	_maxBits=tmp;
	if (!_packedData.readBE(2,tmp)) return;
	_stackLength=uint32_t(tmp)+5;
	if ((8<<_maxBits)+_stackLength>Decompressor::getMaxMemorySize()) return;
	_isValid=true;
}

BLZWDecompressor::~BLZWDecompressor()
{
	// nothing needed
}

bool BLZWDecompressor::isValid() const
{
	return _isValid;
}

bool BLZWDecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool BLZWDecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &BLZWDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-BLZW: BLZW LZW-compressor";
	return name;
}

bool BLZWDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=4;
	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;

	auto readBits=[&](uint32_t bits)->uint32_t
	{
		while (bufBitsLength<bits)
		{
			if (bufOffset>=packedSize)
			{
				streamStatus=false;
				return 0;
			}
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
	auto prefix=std::make_unique<uint32_t[]>(maxCode-258);
	auto suffix=std::make_unique<uint32_t[]>(maxCode-258);
	auto stack=std::make_unique<uint8_t[]>(_stackLength);

	uint32_t freeIndex,codeBits,prevCode,newCode;

	auto suffixLookup=[&](uint32_t code)->uint32_t
	{
		if (code>=freeIndex)
		{
			streamStatus=false;
			return 0;
		}
		return (code<259)?code:suffix[code-259];
	};

	auto insert=[&](uint32_t code)
	{
		uint32_t stackPos=0;
		newCode=suffixLookup(code);
		while (code>=259 && streamStatus)
		{
			if (stackPos+1>=_stackLength)
			{
				streamStatus=false;
				return;
			}
			stack[stackPos++]=newCode;
			code=prefix[code-259];
			newCode=suffixLookup(code);
		}
		stack[stackPos++]=newCode;
		if (!streamStatus || destOffset+stackPos>rawSize)
		{
			streamStatus=false;
		} else {
			while (stackPos) dest[destOffset++]=stack[--stackPos];
		}
	};

	auto init=[&]()
	{
		codeBits=9;
		freeIndex=259;
		prevCode=readBits(codeBits);
		insert(prevCode);
	};

	init();
	while (streamStatus && destOffset!=rawSize)
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
				if (!streamStatus || destOffset>=rawSize)
				{
					streamStatus=false;
				} else {
					dest[destOffset++]=tmp;
				}
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
	return streamStatus && destOffset==rawSize;
}
