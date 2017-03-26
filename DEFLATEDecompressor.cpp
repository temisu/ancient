/* Copyright (C) Teemu Suutari */

#include <stdint.h>
#include <string.h>

#include "DEFLATEDecompressor.hpp"
#include "HuffmanDecoder.hpp"
#include <CRC32.hpp>

static bool Adler32(const Buffer &buffer,size_t offset,size_t len,uint32_t &retValue)
{
	if (!len || offset+len>buffer.size()) return false;
	const uint8_t *ptr=buffer.data()+offset;

	uint32_t s1=1,s2=0;
	for (size_t i=0;i<len;i++)
	{
		s1+=ptr[i];
		if (s1>=65521) s1-=65521;
		s2+=s1;
		if (s2>=65521) s2-=65521;
	}
	retValue=(s2<<16)|s1;
	return true;
}

bool DEFLATEDecompressor::detectHeader(uint32_t hdr)
{
	return ((hdr>>16)==0x1f8b);
}

bool DEFLATEDecompressor::detectHeaderXPK(uint32_t hdr)
{
	return (hdr==FourCC('GZIP'));
}

bool DEFLATEDecompressor::detectGZIP()
{
	if (_packedData.size()<18) return false;
	uint32_t hdr;
	if (!_packedData.readBE(0,hdr)) return false;
	if (!detectHeader(hdr)) return false;

	uint8_t cm;
	if (!_packedData.read(2,cm)) return false;
	if (cm!=8) return false;

	uint8_t flags;
	if (!_packedData.read(3,flags)) return false;
	if (flags&0xe0) return false;
	
	uint32_t currentOffset=10;

	if (flags&4)
	{
		uint16_t xlen;
		if (!_packedData.readLE(currentOffset,xlen)) return false;
		currentOffset+=uint32_t(xlen)+2;
	}
	
	auto skipString=[&]()->bool
	{
		uint8_t ch;
		do {
			if (!_packedData.read(currentOffset,ch)) return false;
			currentOffset++;
		} while (ch);
		return true;
	};
	
	if (flags&8)
		if (!skipString()) return false; // FNAME
	if (flags&16)
		if (!skipString()) return false; // FCOMMENT

	if (flags&2) currentOffset+=2; // FHCRC
	_packedOffset=currentOffset;

	if (currentOffset+8>_packedData.size()) return false;
	_packedSize=uint32_t(_packedData.size())-8;

	if (_exactSizeKnown)
	{
		if (!_packedData.readLE(_packedData.size()-8,_rawCRC)) return false;
		uint32_t tmp;
		if (!_packedData.readLE(_packedData.size()-4,tmp)) return false;
		_rawSize=tmp;
	}

	_type=Type::GZIP;
	return true;
}

bool DEFLATEDecompressor::detectZLib()
{
	if (_packedData.size()<6) return false;

	// no knowledge about rawSize, before decompression
	// packedSize told by decompressor
	_packedSize=uint32_t(_packedData.size())-4;
	_packedOffset=2;

	// really it is adler32, but we can use the same name
	if (!_packedData.readBE(_packedSize,_rawCRC)) return false;

	uint8_t cm;
	if (!_packedData.read(0,cm)) return false;
	if ((cm&0xf)!=8) return false;
	if ((cm&0xf0)>0x70) return false;

	uint8_t flags;
	if (!_packedData.read(1,flags)) return false;
	if (flags&0x20)
	{
		if (_packedSize<4) return false;
		_packedOffset+=4;
	}

	if (((uint16_t(cm)<<8)|uint16_t(flags))%31) return false;

	_type=Type::ZLib;
	return true;
}

DEFLATEDecompressor::DEFLATEDecompressor(const Buffer &packedData,bool exactSizeKnown) :
	_packedData(packedData),
	_exactSizeKnown(exactSizeKnown)
{
	_isValid=detectGZIP();
}

DEFLATEDecompressor::DEFLATEDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state) :
	_packedData(packedData)
{
	if (!detectZLib())
	{
		_packedSize=uint32_t(packedData.size());
		_packedOffset=0;
		_type=Type::Raw;
	}
	_isValid=true;
}

DEFLATEDecompressor::DEFLATEDecompressor(const Buffer &packedData,uint32_t packedSize,uint32_t rawSize) :
	_packedData(packedData)
{
	_packedSize=uint32_t(packedData.size());
	_packedOffset=0;
	_rawSize=rawSize;
	_type=Type::Raw;
	_isValid=true;
}

DEFLATEDecompressor::~DEFLATEDecompressor()
{
	// nothing needed
}

bool DEFLATEDecompressor::isValid() const
{
	return _isValid;
}

bool DEFLATEDecompressor::verifyPacked() const
{
	// only hdr could have CRC, but in practice this is not really used
	return _isValid;
}

bool DEFLATEDecompressor::verifyRaw(const Buffer &rawData) const
{
	if (_type==Type::GZIP && _exactSizeKnown)
	{
		if (rawData.size()<_rawSize) return false;
		uint32_t crc;
		return CRC32(rawData,0,_rawSize,crc) && crc==_rawCRC;
	} else if (_type==Type::ZLib) {
		uint32_t adler;
		return Adler32(rawData,0,rawData.size(),adler) && adler==_rawCRC;
	} else return true;
}

const std::string &DEFLATEDecompressor::getName() const
{
	if (!_isValid) return Decompressor::getName();
	static std::string names[3]={
		"gzip: Deflate",
		"zlib: Deflate",
		"raw: Deflate"};
	return names[static_cast<uint32_t>(_type)];
}

const std::string &DEFLATEDecompressor::getSubName() const
{
	if (!_isValid) return XPKDecompressor::getSubName();
	static std::string name="XPK-GZIP: Deflate";
	return name;
}

size_t DEFLATEDecompressor::getPackedSize() const
{
	// no way to know before decompressing
	if (!_isValid) return 0;
	if (_type==Type::GZIP)
	{
		return _packedSize+8;
	} else if (_type==Type::ZLib) {
		return _packedSize+2;
	} else  return _packedSize;
}


size_t DEFLATEDecompressor::getRawSize() const
{
	// same thing, decompression needed first
	if (!_isValid) return 0;
	return _rawSize;
}

bool DEFLATEDecompressor::decompress(Buffer &rawData)
{
	size_t rawSize=_rawSize;
	if (rawSize<rawData.size()) rawSize=rawData.size();

	if (!_isValid || rawData.size()<rawSize) return false;

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
			if (bufOffset>=_packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent=bufPtr[bufOffset++];
			bufBitsLength=8;
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
			if (bufOffset>=_packedSize)
			{
				streamStatus=false;
				return 0;
			}
			bufBitsContent|=uint32_t(bufPtr[bufOffset++])<<bufBitsLength;
			bufBitsLength+=8;
		}
		uint32_t ret=bufBitsContent&((1<<count)-1);
		bufBitsContent>>=count;
		bufBitsLength-=count;
		return ret;
	};

	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	bool final;
	do {
		final=readBit();
		uint8_t blockType=readBits(2);
		if (!blockType)
		{
			bufBitsLength=0;
			bufBitsContent=0;
			uint16_t len,nlen;
			if (!_packedData.readLE(bufOffset,len)) streamStatus=false;
			if (!_packedData.readLE(bufOffset+2,nlen)) streamStatus=false;
			bufOffset+=4;
			if (len!=uint16_t(~nlen)) streamStatus=false;
			if (!streamStatus || bufOffset+len>_packedSize || destOffset+len>rawSize)
			{
				streamStatus=false;
			} else {
				::memcpy(&dest[destOffset],&bufPtr[bufOffset],len);
				bufOffset+=len;
				destOffset+=len;
			}
		} else if (blockType==1 || blockType==2) {
			typedef HuffmanDecoder<int32_t,-1,0> DEFLATEDecoder;
			DEFLATEDecoder llDecoder;
			DEFLATEDecoder distanceDecoder;

			if (blockType==1)
			{
				for (uint32_t i=0;i<24;i++) llDecoder.insert(HuffmanCode<int32_t>{7,i,int32_t(i+256)});
				for (uint32_t i=0;i<144;i++) llDecoder.insert(HuffmanCode<int32_t>{8,i+0x30,int32_t(i)});
				for (uint32_t i=0;i<8;i++) llDecoder.insert(HuffmanCode<int32_t>{8,i+0xc0,int32_t(i+280)});
				for (uint32_t i=0;i<112;i++) llDecoder.insert(HuffmanCode<int32_t>{9,i+0x190,int32_t(i+144)});

				for (uint32_t i=0;i<30;i++) distanceDecoder.insert(HuffmanCode<int32_t>{5,i,int32_t(i)});
			} else {
				uint32_t hlit=readBits(5)+257;
				// lets just error here, it is simpler
				if (hlit>=287)
				{
					streamStatus=false;
					break;
				}
				uint32_t hdist=readBits(5)+1;
				uint32_t hclen=readBits(4)+4;

				uint8_t lengthTable[19];
				for (uint32_t i=0;i<19;i++) lengthTable[i]=0;
				static const uint8_t lengthTableOrder[19]={
					16,17,18, 0, 8, 7, 9, 6,
					10, 5,11, 4,12, 3,13, 2,
					14, 1,15};
				for (uint32_t i=0;i<hclen;i++) lengthTable[lengthTableOrder[i]]=readBits(3);

				auto createHuffmanTable=[&](DEFLATEDecoder &dec,const uint8_t *bitLengths,uint32_t bitTableLength)
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

				DEFLATEDecoder bitLengthDecoder;
				createHuffmanTable(bitLengthDecoder,lengthTable,19); // 19 and not hclen due to reordering

				// can the previous code flow from ll to distance table?
				// specification does not say and treats the two almost as combined.
				// So let previous code flow

				uint8_t llTableBits[287];
				for (uint32_t i=0;i<287;i++) llTableBits[i]=0;
				uint8_t distanceTableBits[32];
				for (uint32_t i=0;i<32;i++) distanceTableBits[i]=0;

				uint8_t prevValue=0;
				uint32_t i=0;
				while (i<hlit+hdist&&streamStatus)
				{
					auto insert=[&](uint8_t value)
					{
						if (i>=hlit+hdist)
						{
							streamStatus=false;
							return;
						}
						if (i>=hlit) distanceTableBits[i-hlit]=value;
							else llTableBits[i]=value;
						prevValue=value;
						i++;
					};

					int32_t code=bitLengthDecoder.decode(readBit);
					if (code<0)
					{
						streamStatus=false;
					} else if (code<16) {
						insert(code);
					} else switch (code) {
						case 16:
						if (i) 
						{
							uint32_t count=readBits(2)+3;
							for (uint32_t j=0;j<count;j++) insert(prevValue);
						} else streamStatus=false;
						break;

						case 17:
						for (uint32_t count=readBits(3)+3;count;count--) insert(0);
						break;

						case 18:
						for (uint32_t count=readBits(7)+11;count;count--) insert(0);
						break;

						default:
						streamStatus=false;
						break;
					}
					
				}

				createHuffmanTable(llDecoder,llTableBits,hlit);
				createHuffmanTable(distanceDecoder,distanceTableBits,hdist);
			}

			// and now decode
			while (streamStatus)
			{
				int32_t code=llDecoder.decode(readBit);
				if (code<0)
				{
					streamStatus=false;
				} else if (code<256) {
					if (destOffset>=rawSize)
					{
						streamStatus=false;
					} else dest[destOffset++]=code;
				} else if (code==256) {
					break;
				} else {
					static const uint32_t lengthAdditions[29]={
						3,4,5,6,7,8,9,10,
						11,13,15,17,
						19,23,27,31,
						35,43,51,59,
						67,83,99,115,
						131,163,195,227,
						258};
					static const uint32_t lengthBits[29]={
						0,0,0,0,0,0,0,0,
						1,1,1,1,2,2,2,2,
						3,3,3,3,4,4,4,4,
						5,5,5,5,0};
					uint32_t count=readBits(lengthBits[code-257])+lengthAdditions[code-257];
					int32_t distCode=distanceDecoder.decode(readBit);
					if (distCode<0 || distCode>29)
					{
						streamStatus=false;
						break;
					}
					static const uint32_t distanceAdditions[30]={
						1,2,3,4,5,7,9,13,
						0x11,0x19,0x21,0x31,0x41,0x61,0x81,0xc1,
						0x101,0x181,0x201,0x301,0x401,0x601,0x801,0xc01,
						0x1001,0x1801,0x2001,0x3001,0x4001,0x6001};
					static const uint32_t distanceBits[30]={
						0,0,0,0,1,1,2,2,
						3,3,4,4,5,5,6,6,
						7,7,8,8,9,9,10,10,
						11,11,12,12,13,13};
					uint32_t distance=readBits(distanceBits[distCode])+distanceAdditions[distCode];

					if (!streamStatus || distance>destOffset || destOffset+count>rawSize)
					{
						streamStatus=false;
					} else {
						for (uint32_t i=0;i<count;i++,destOffset++)
							dest[destOffset]=dest[destOffset-distance];
					}
				}
			}
		} else {
			streamStatus=false;
		}
	} while (!final && streamStatus);

	if (streamStatus && !_rawSize)
	{
		_rawSize=destOffset;
	}
	bool ret=(streamStatus && _rawSize==destOffset);
	if (ret && _packedSize>bufOffset)
	{
		_packedSize=bufOffset;
	}
	return ret;
}
