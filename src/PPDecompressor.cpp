/* Copyright (C) Teemu Suutari */

#include "PPDecompressor.hpp"

PPDecompressor::PPState::PPState(uint32_t mode) :
	_cachedMode(mode)
{
	// nothing needed
}

PPDecompressor::PPState::~PPState()
{
	// nothing needed
}

bool PPDecompressor::detectHeader(uint32_t hdr)
{
	return (hdr==FourCC('PP11') || hdr==FourCC('PP20'));
}

bool PPDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return hdr==FourCC('PWPK');
}

bool PPDecompressor::isRecursive()
{
	return false;
}

std::unique_ptr<XPKDecompressor> PPDecompressor::create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state)
{
	return std::make_unique<PPDecompressor>(hdr,packedData,state);
}

PPDecompressor::PPDecompressor(const Buffer &packedData,bool exactSizeKnown) :
	_packedData(packedData)
{
	if (!exactSizeKnown) return;		// no scanning supports
	if (packedData.size()<0x10) return;
	_dataStart=_packedData.size()-4;

	uint32_t hdr;
	if (!packedData.readBE(0,hdr)) return;
	if (!detectHeader(hdr)) return;
	uint32_t mode;
	if (!packedData.readBE(4,mode)) return;
	if (mode!=0x9090909 && mode!=0x90a0a0a && mode!=0x90a0b0b && mode!=0x90a0c0c && mode!=0x90a0c0d) return;
	for (uint32_t i=0;i<4;i++)
	{
		_modeTable[i]=mode>>24;
		mode<<=8;
	}

	uint32_t tmp;
	if (!packedData.readBE(_dataStart,tmp)) return;

	_rawSize=tmp>>8;
	_startShift=tmp&0xff;
	if (!_rawSize || _startShift>=0x20) return;
	if (_rawSize>getMaxRawSize()) return;
	_isValid=true;
}

PPDecompressor::PPDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr)) return;
	if (packedData.size()<0x10) return;
	_dataStart=_packedData.size()-4;

	uint32_t mode;
	if (state.get())
	{
		mode=static_cast<PPState*>(state.get())->_cachedMode;
	} else {
		if (!packedData.readBE(_dataStart,mode)) return;
		if (mode>4) return;
		state.reset(new PPState(mode));
		_dataStart-=4;
	}

	static const uint32_t modeMap[5]={0x9090909,0x90a0a0a,0x90a0b0b,0x90a0c0c,0x90a0c0d};
	mode=modeMap[mode];
	for (uint32_t i=0;i<4;i++)
	{
		_modeTable[i]=mode>>24;
		mode<<=8;
	}

	uint32_t tmp;
	if (!packedData.readBE(_dataStart,tmp)) return;

	_rawSize=tmp>>8;
	_startShift=tmp&0xff;
	if (!_rawSize || _startShift>=0x20) return;
	if (_rawSize>getMaxRawSize()) return;

	_isXPK=true;
	_isValid=true;
}

PPDecompressor::~PPDecompressor()
{
	// nothing needed
}

bool PPDecompressor::isValid() const
{
	return _isValid;
}

bool PPDecompressor::verifyPacked() const
{
	// nothing ...
	return _isValid;
}

bool PPDecompressor::verifyRaw(const Buffer &rawData) const
{
	// no CRC
	return _isValid;
}

const std::string &PPDecompressor::getName() const
{
	if (!_isValid) return Decompressor::getName();
	static std::string name="PP: PowerPacker";
	return name;
}

const std::string &PPDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-PWPK: PowerPacker";
	return name;
}

size_t PPDecompressor::getPackedSize() const
{
	return 0;
}

size_t PPDecompressor::getRawSize() const
{
	if (!_isValid) return 0;
	return _rawSize;
}

bool PPDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_rawSize) return false;

	// Stream reading
	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_dataStart;

	uint32_t bufBitsContent=0;
	uint8_t bufBitsLength=0;
	size_t minBufOffset=_isXPK?0:8;

	auto fillBuffer=[&]()->bool {
		if (bufOffset+3<=minBufOffset) return false;
		for (uint32_t i=0;i<4;i++) bufBitsContent=(uint32_t(bufPtr[--bufOffset])<<24)|(bufBitsContent>>8);
		bufBitsLength=32;
		return true;
	};
	if (!fillBuffer()) return false;
	bufBitsContent>>=_startShift;
	bufBitsLength-=_startShift;

	auto readBit=[&]()->uint8_t
	{
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (!fillBuffer())
			{
				streamStatus=false;
				return 0;
			}
		}
		uint8_t ret=bufBitsContent&1;
		bufBitsContent>>=1;
		bufBitsLength--;
		return ret;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<count;i++) ret=(ret<<1)|readBit();
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=_rawSize;

	while (streamStatus)
	{
		if (!readBit())
		{
			uint32_t count=1;
			// This does not make much sense I know. But it is what it is...
			while (streamStatus)
			{
				uint32_t tmp=readBits(2);
				count+=tmp;
				if (tmp<3) break;
			}
			if (!streamStatus || destOffset<count)
			{
				streamStatus=false;
			} else {
				for (uint32_t i=0;i<count;i++) dest[--destOffset]=readBits(8);
			}
		}
		if (!destOffset) break;
		uint32_t modeIndex=readBits(2);
		uint32_t count,distance;
		if (modeIndex==3)
		{
			distance=readBits(readBit()?_modeTable[modeIndex]:7)+1;
			// ditto
			count=5;
			while (streamStatus)
			{
				uint32_t tmp=readBits(3);
				count+=tmp;
				if (tmp<7) break;
			}
		} else {
			count=modeIndex+2;
			distance=readBits(_modeTable[modeIndex])+1;
		}
		if (destOffset<count || destOffset+distance>_rawSize)
		{
			streamStatus=false;
		} else {
			distance+=destOffset;
			for (uint32_t i=0;i<count;i++) dest[--destOffset]=dest[--distance];
		}
	}

	return streamStatus && !destOffset;
}

bool PPDecompressor::decompress(Buffer &rawData,const Buffer &previousData)
{
	if (_rawSize!=rawData.size()) return false;
	return decompress(rawData);
}
