/* Copyright (C) Teemu Suutari */

#include "IMPDecompressor.hpp"
#include "HuffmanDecoder.hpp"

static bool readIMPHeader(uint32_t hdr,uint32_t &addition) noexcept
{
	switch (hdr)
	{
		case FourCC('IMP!'):
		case FourCC('ATN!'):
		addition=7;
		return true;

		case FourCC('BDPI'):
		addition=0x6e8;
		return true;

		case FourCC('CHFI'):
		addition=0xfe4;
		return true;

		// I haven't got these files to be sure what is the addition
		case FourCC('Dupa'):
		case FourCC('EDAM'):
		case FourCC('FLT!'):
		case FourCC('M.H.'):
		case FourCC('PARA'):
		case FourCC('RDC9'):
		addition=0; 		// disable checksum for now
		return true;

		default:
		return false;
	}
}

bool IMPDecompressor::detectHeader(uint32_t hdr) noexcept
{
	uint32_t dummy;
	return readIMPHeader(hdr,dummy);
}

bool IMPDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('IMPL');
}

std::unique_ptr<Decompressor> IMPDecompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<IMPDecompressor>(packedData,verify);
}

std::unique_ptr<XPKDecompressor> IMPDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<IMPDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

IMPDecompressor::IMPDecompressor(const Buffer &packedData,bool verify) :
	_packedData(packedData)
{
	uint32_t hdr=packedData.readBE32(0);
	uint32_t checksumAddition;
	if (!readIMPHeader(hdr,checksumAddition) || packedData.size()<0x32) throw InvalidFormatError();

	_rawSize=packedData.readBE32(4);
	_endOffset=packedData.readBE32(8);
	if ((_endOffset&1) || _endOffset<0xc || _endOffset+0x32<packedData.size() ||
		!_rawSize || !_endOffset ||
		_rawSize>getMaxRawSize() || _endOffset>getMaxPackedSize()) throw InvalidFormatError();
	uint32_t checksum=packedData.readBE32(_endOffset+0x2e);
	if (verify && checksumAddition)
	{
		// size is divisible by 2
		uint32_t sum=checksumAddition;
		for (uint32_t i=0;i<_endOffset+0x2e;i+=2)
		{
			// TODO: slow, optimize
			uint16_t tmp=_packedData.readBE16(i);
			sum+=uint32_t(tmp);
		}
		if (checksum!=sum) throw InvalidFormatError();
	}
}

IMPDecompressor::IMPDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData)
{
	if (!detectHeaderXPK(hdr) || packedData.size()<0x2e) throw InvalidFormatError();

	_rawSize=packedData.readBE32(4);
	_endOffset=packedData.readBE32(8);
	if ((_endOffset&1) || _endOffset<0xc || _endOffset+0x2e<packedData.size()) throw InvalidFormatError();
	_isXPK=true;
}

IMPDecompressor::~IMPDecompressor()
{
	// nothing needed
}

const std::string &IMPDecompressor::getName() const noexcept
{
	static std::string name="IMP: File Imploder";
	return name;
}

const std::string &IMPDecompressor::getSubName() const noexcept
{
	static std::string name="XPK-IMPL: File Imploder";
	return name;
}

size_t IMPDecompressor::getPackedSize() const noexcept
{
	return _endOffset+0x32;
}

size_t IMPDecompressor::getRawSize() const noexcept
{
	return _rawSize;
}

void IMPDecompressor::decompressImpl(Buffer &rawData,bool verify)
{
	if (rawData.size()<_rawSize) throw DecompressionError();

	uint8_t markerByte=_packedData.read8(_endOffset+16);

	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_endOffset;
	if (!(markerByte&0x80)) bufOffset--;

	uint8_t bufBitsContent=_packedData.read8(_endOffset+17);
	uint8_t bufBitsLength=7;
	// the anchor-bit does not seem always to be at the correct place
	for (uint32_t i=0;i<7;i++)
		if (bufBitsContent&(1<<i)) break;
			else bufBitsLength--;

	// streamreader with funny ordering
	auto sourceOffset=[&](size_t i)->size_t
	{
		if (i>=12)
		{
			return i;
		} else {
			if (i<4)
			{
				return i+_endOffset+8;
			} else if (i<8) {
				return i+_endOffset;
			} else {
				return i+_endOffset-8;
			}
		}
	};

	auto readBit=[&]()->uint8_t
	{
		if (!bufBitsLength)
		{
			if (!bufOffset) throw DecompressionError();
			bufBitsContent=bufPtr[sourceOffset(--bufOffset)];
			bufBitsLength=8;
		}
		uint8_t ret=bufBitsContent>>7;
		bufBitsContent<<=1;
		bufBitsLength--;
		return ret;
	};

	auto readBits=[&](uint32_t count)->uint32_t
	{
		uint32_t ret=0;
		for (uint32_t i=0;i<count;i++) ret=(ret<<1)|uint32_t(readBit());
		return ret;
	};

	auto readByte=[&]()->uint8_t
	{
		if (!bufOffset) throw DecompressionError();
		return bufPtr[sourceOffset(--bufOffset)];
	};
	

	// tables
	uint16_t distanceValues[2][4];
	for (uint32_t i=0;i<8;i++)
		distanceValues[i>>2][i&3]=_packedData.readBE16(_endOffset+18+i*2);
	uint8_t distanceBits[3][4];
	for (uint32_t i=0;i<12;i++)
		distanceBits[i>>2][i&3]=_packedData.read8(_endOffset+34+i);

	// length, distance & literal counts are all intertwined
	HuffmanDecoder<uint8_t,0xffU,5> lldDecoder
	{
		HuffmanCode<uint8_t>{1,0b00000,0},
		HuffmanCode<uint8_t>{2,0b00010,1},
		HuffmanCode<uint8_t>{3,0b00110,2},
		HuffmanCode<uint8_t>{4,0b01110,3},
		HuffmanCode<uint8_t>{5,0b11110,4},
		HuffmanCode<uint8_t>{5,0b11111,5}
	};

	HuffmanDecoder<uint8_t,0xffU,2> lldDecoder2
	{
		HuffmanCode<uint8_t>{1,0b00,0},
		HuffmanCode<uint8_t>{2,0b10,1},
		HuffmanCode<uint8_t>{2,0b11,2}
	};

	// finally loop

	uint8_t *dest=rawData.data();
	size_t destOffset=_rawSize;

	uint32_t litLength=_packedData.readBE32(_endOffset+12);

	for (;;)
	{
		if (destOffset<litLength) throw DecompressionError();
		for (uint32_t i=0;i<litLength;i++) dest[--destOffset]=readByte();

		if (!destOffset) break;

		// now the intertwined Huffman table reads.
		uint32_t i0=lldDecoder.decode(readBit);
		uint32_t selector=(i0<4)?i0:3;
		uint32_t count=i0+2;
		if (count==6)
		{
			count+=readBits(3);
		} else if (count==7) {
			count=readByte();
			// why this is error? (Well, it just is)
			if (!count) throw DecompressionError();
		}

		static const uint8_t literalLengths[4]={6,10,10,18};
		static const uint8_t literalBits[3][4]={
			{1,1,1,1},
			{2,3,3,4},
			{4,5,7,14}};
		uint32_t i1=lldDecoder2.decode(readBit);
		litLength=i1+i1;
		if (litLength==4)
		{
			litLength=literalLengths[selector];
		}
		litLength+=readBits(literalBits[i1][selector]);

		uint32_t i2=lldDecoder2.decode(readBit);
		uint32_t distance=1+((i2)?distanceValues[i2-1][selector]:0)+readBits(distanceBits[i2][selector]);

		if (destOffset<count || destOffset+distance>_rawSize) throw DecompressionError();
		distance+=destOffset;
		for (uint32_t i=0;i<count;i++) dest[--destOffset]=dest[--distance];
	}
}

void IMPDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	if (_rawSize!=rawData.size()) throw DecompressionError();
	return decompressImpl(rawData,verify);
}

Decompressor::Registry<IMPDecompressor> IMPDecompressor::_registration;
XPKDecompressor::Registry<IMPDecompressor> IMPDecompressor::_XPKregistration;
