/* Copyright (C) Teemu Suutari */

#include "IMPDecompressor.hpp"
#include "HuffmanDecoder.hpp"

static bool readIMPHeader(uint32_t hdr,uint32_t &addition)
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
		addition=7;
		return true;

		default:
		return false;
	}
}

bool IMPDecompressor::detectHeader(uint32_t hdr)
{
	uint32_t dummy;
	return readIMPHeader(hdr,dummy);
}

IMPDecompressor::IMPDecompressor(const Buffer &packedData) :
	Decompressor(packedData),
	_isValid(false)
{
	if (packedData.size()<0x32) return;
	uint32_t hdr;
	if (!packedData.read(0,hdr)) return;
	if (!readIMPHeader(hdr,_checksumAddition)) return;

	if (!packedData.read(4,_rawSize)) return;
	if (!packedData.read(8,_endOffset)) return;
	if ((_endOffset&1) || _endOffset<0xc || _endOffset+0x32<packedData.size()) return;
	if (!packedData.read(_endOffset+0x2e,_checksum)) return;
	_isValid=true;
}

IMPDecompressor::~IMPDecompressor()
{
	// nothing needed
}

bool IMPDecompressor::isValid() const
{
	return _isValid;
}

bool IMPDecompressor::verifyPacked() const
{
	// size is divisible by 2
	uint32_t sum=_checksumAddition;
	for (uint32_t i=0;i<_endOffset+0x2e;i+=2)
	{
		uint16_t tmp;
		if (!_packedData.read(i,tmp)) return false;
		sum+=uint32_t(tmp);
	}
	return _checksum==sum;
}

bool IMPDecompressor::verifyRaw(const Buffer &rawData) const
{
	// no CRC
	return _isValid;
}

size_t IMPDecompressor::getRawSize() const
{
	if (!_isValid) return 0;
	return _rawSize;
}

bool IMPDecompressor::decompress(Buffer &rawData)
{
	if (!_isValid || rawData.size()<_rawSize) return false;

	uint8_t markerByte;
	if (!_packedData.read(_endOffset+16,markerByte)) return false;

	bool streamStatus=true;
	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=_endOffset;
	if (!(markerByte&0x80)) bufOffset--;

	uint8_t bufBitsContent;
	if (!_packedData.read(_endOffset+17,bufBitsContent)) return false;
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
		if (!streamStatus) return 0;
		if (!bufBitsLength)
		{
			if (!bufOffset)
			{
				streamStatus=false;
				return 0;
			}
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
		if (!streamStatus || !bufOffset)
		{
			streamStatus=false;
			return 0;
		}
		return bufPtr[sourceOffset(--bufOffset)];
	};
	

	// tables
	uint16_t distanceValues[2][4];
	for (uint32_t i=0;i<8;i++)
		if (!_packedData.read(_endOffset+18+i*2,distanceValues[i>>2][i&3])) return false;
	uint8_t distanceBits[3][4];
	for (uint32_t i=0;i<12;i++)
		if (!_packedData.read(_endOffset+34+i,distanceBits[i>>2][i&3])) return false;

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

	uint32_t litLength;
	if (!_packedData.read(_endOffset+12,litLength)) return false;

	while (streamStatus)
	{
		if (destOffset<litLength)
		{
			streamStatus=false;
			break;
		} else {
			for (uint32_t i=0;i<litLength;i++) dest[--destOffset]=readByte();
		}

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
			// why this is error?
			if (!count)
			{
				streamStatus=false;
				break;
			}
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
