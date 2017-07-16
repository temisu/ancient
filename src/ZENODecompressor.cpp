/* Copyright (C) Teemu Suutari */

#include "ZENODecompressor.hpp"

bool ZENODecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('ZENO');
}


bool ZENODecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> ZENODecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<ZENODecompressor>(hdr,packedData,state);
}

ZENODecompressor::ZENODecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	// first 4 bytes is checksum for password. It needs to be zero
	uint32_t sum;
	if (!_packedData.readBE(0,sum) || sum) return;
	uint8_t tmp;
	if (!_packedData.read(4,tmp) || tmp<9 || tmp>24) return;
	_maxBits=tmp;
	if (!_packedData.read(5,tmp)) return;
	_startOffset=uint32_t(tmp)+6;
	if (_startOffset>=_packedData.size()) return;
	if ((5U<<_maxBits)>Decompressor::getMaxMemorySize()) return;
	_isValid=true;
}

ZENODecompressor::~ZENODecompressor()
{
	// nothing needed
}

bool ZENODecompressor::isValid() const
{
	return _isValid;
}

bool ZENODecompressor::verifyPacked() const
{
	// nothing can be done
	return _isValid;
}

bool ZENODecompressor::verifyRaw(const Buffer &rawData) const
{
	// nothing can be done
	return _isValid;
}

const std::string &ZENODecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-ZENO: LZW-compressor";
	return name;
}

bool ZENODecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (!_isValid) return false;

	// Stream reading
	bool streamStatus=true;
	size_t packedSize=_packedData.size();
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_startOffset;
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
	uint32_t stackLength=5000;				// magic constant
	auto prefix=std::make_unique<uint32_t[]>(maxCode-258);
	auto suffix=std::make_unique<uint8_t[]>(maxCode-258);
	auto stack=std::make_unique<uint8_t[]>(stackLength);

	uint32_t freeIndex,codeBits,prevCode,newCode;

	auto init=[&]()
	{
		codeBits=9;
		freeIndex=258;
	};

	init();
	prevCode=readBits(9);
	newCode=prevCode;
	suffix[freeIndex-258]=0;
	prefix[freeIndex-258]=0;
	freeIndex++;
	if (!streamStatus || destOffset>=rawSize)
	{
		streamStatus=false;
	} else {
		dest[destOffset++]=newCode;
	}
	
	while (streamStatus && destOffset!=rawSize)
	{
		if (freeIndex+3>=(1U<<codeBits) && codeBits<_maxBits) codeBits++;
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

			default:
			{
				uint32_t stackPos=0;
				uint32_t tmp=code;
				if (tmp==freeIndex)
				{
					stack[stackPos++]=newCode;
					tmp=prevCode;
				}
				if (tmp>=258)
				{
					do {
						if (stackPos+1>=stackLength || tmp>=freeIndex)
						{
							streamStatus=false;
							break;
						}
						stack[stackPos++]=suffix[tmp-258];
						tmp=prefix[tmp-258];
					} while (tmp>=258);
					stack[stackPos++]=newCode=tmp;
					if (!streamStatus || destOffset+stackPos>rawSize)
					{
						streamStatus=false;
					} else {
						while (stackPos) dest[destOffset++]=stack[--stackPos];
					}
				} else {
					newCode=tmp;
					if (destOffset+stackPos>=rawSize)
					{
						streamStatus=false;
						break;
					}
					dest[destOffset++]=tmp;
					if (stackPos) dest[destOffset++]=stack[0];
				}
			}
			if (freeIndex<maxCode)
			{
				suffix[freeIndex-258]=newCode;
				prefix[freeIndex-258]=prevCode;
				freeIndex++;
			}
			prevCode=code;
			break;
		}
		if (doExit) break;
	}
	return streamStatus && destOffset==rawSize;
}
