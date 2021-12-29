/* Copyright (C) Teemu Suutari */

// This is really quick and dirty. Works though

#include <memory>

#include <cstdint>
#include <cstring>

#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>

#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

#include "common/Buffer.hpp"
#include "common/SubBuffer.hpp"

#define FUZZY_BLOCK_CUT_THRESHOLD (256)

namespace ancient::internal
{

class VectorBuffer : public Buffer
{
public:
	VectorBuffer();

	virtual ~VectorBuffer() override final;

	virtual const uint8_t *data() const noexcept override final;
	virtual uint8_t *data() override final;
	virtual size_t size() const noexcept override final;

	virtual bool isResizable() const noexcept override final;
	virtual void resize(size_t newSize) override final;

private:
	std::vector<uint8_t>  _data;
};

VectorBuffer::VectorBuffer()
{
	// nothing needed
}

VectorBuffer::~VectorBuffer()
{
	// nothing needed
}

const uint8_t *VectorBuffer::data() const noexcept
{
	return _data.data();
}

uint8_t *VectorBuffer::data()
{
	return _data.data();
}

size_t VectorBuffer::size() const noexcept
{
	return _data.size();
}

bool VectorBuffer::isResizable() const noexcept
{
	return true;
}

void VectorBuffer::resize(size_t newSize) 
{
	return _data.resize(newSize);
}

std::unique_ptr<Buffer> readFile(const std::string &fileName)
{

	std::unique_ptr<Buffer> ret=std::make_unique<VectorBuffer>();
	std::ifstream file(fileName.c_str(),std::ios::in|std::ios::binary);
	bool success=false;
	if (file.is_open())
	{
		file.seekg(0,std::ios::end);
		size_t length=size_t(file.tellg());
		file.seekg(0,std::ios::beg);
		ret->resize(length);
		file.read(reinterpret_cast<char*>(ret->data()),length);
		success=bool(file);
		if (!success) ret->resize(0);
		file.close();
	}
	if (!success)
	{
		fprintf(stderr,"Could not read file %s\n",fileName.c_str());
	}
	return ret;
}

bool writeFile(const std::string &fileName,const Buffer &content)
{
	bool ret=false;
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open()) {
		file.write(reinterpret_cast<const char*>(content.data()),content.size());
		ret=bool(file);
		file.close();
	}
	if (!ret)
	{
		fprintf(stderr,"Could not write file %s\n",fileName.c_str());
	}
	return ret;
}

uint16_t RNCCRC(const uint8_t *buffer,size_t len)
{
	// bit reversed 16bit CRC with 0x8005 polynomial
	static const uint16_t CRCTable[256]={
		0x0000,0xc0c1,0xc181,0x0140,0xc301,0x03c0,0x0280,0xc241,0xc601,0x06c0,0x0780,0xc741,0x0500,0xc5c1,0xc481,0x0440,
		0xcc01,0x0cc0,0x0d80,0xcd41,0x0f00,0xcfc1,0xce81,0x0e40,0x0a00,0xcac1,0xcb81,0x0b40,0xc901,0x09c0,0x0880,0xc841,
		0xd801,0x18c0,0x1980,0xd941,0x1b00,0xdbc1,0xda81,0x1a40,0x1e00,0xdec1,0xdf81,0x1f40,0xdd01,0x1dc0,0x1c80,0xdc41,
		0x1400,0xd4c1,0xd581,0x1540,0xd701,0x17c0,0x1680,0xd641,0xd201,0x12c0,0x1380,0xd341,0x1100,0xd1c1,0xd081,0x1040,
		0xf001,0x30c0,0x3180,0xf141,0x3300,0xf3c1,0xf281,0x3240,0x3600,0xf6c1,0xf781,0x3740,0xf501,0x35c0,0x3480,0xf441,
		0x3c00,0xfcc1,0xfd81,0x3d40,0xff01,0x3fc0,0x3e80,0xfe41,0xfa01,0x3ac0,0x3b80,0xfb41,0x3900,0xf9c1,0xf881,0x3840,
		0x2800,0xe8c1,0xe981,0x2940,0xeb01,0x2bc0,0x2a80,0xea41,0xee01,0x2ec0,0x2f80,0xef41,0x2d00,0xedc1,0xec81,0x2c40,
		0xe401,0x24c0,0x2580,0xe541,0x2700,0xe7c1,0xe681,0x2640,0x2200,0xe2c1,0xe381,0x2340,0xe101,0x21c0,0x2080,0xe041,
		0xa001,0x60c0,0x6180,0xa141,0x6300,0xa3c1,0xa281,0x6240,0x6600,0xa6c1,0xa781,0x6740,0xa501,0x65c0,0x6480,0xa441,
		0x6c00,0xacc1,0xad81,0x6d40,0xaf01,0x6fc0,0x6e80,0xae41,0xaa01,0x6ac0,0x6b80,0xab41,0x6900,0xa9c1,0xa881,0x6840,
		0x7800,0xb8c1,0xb981,0x7940,0xbb01,0x7bc0,0x7a80,0xba41,0xbe01,0x7ec0,0x7f80,0xbf41,0x7d00,0xbdc1,0xbc81,0x7c40,
		0xb401,0x74c0,0x7580,0xb541,0x7700,0xb7c1,0xb681,0x7640,0x7200,0xb2c1,0xb381,0x7340,0xb101,0x71c0,0x7080,0xb041,
		0x5000,0x90c1,0x9181,0x5140,0x9301,0x53c0,0x5280,0x9241,0x9601,0x56c0,0x5780,0x9741,0x5500,0x95c1,0x9481,0x5440,
		0x9c01,0x5cc0,0x5d80,0x9d41,0x5f00,0x9fc1,0x9e81,0x5e40,0x5a00,0x9ac1,0x9b81,0x5b40,0x9901,0x59c0,0x5880,0x9841,
		0x8801,0x48c0,0x4980,0x8941,0x4b00,0x8bc1,0x8a81,0x4a40,0x4e00,0x8ec1,0x8f81,0x4f40,0x8d01,0x4dc0,0x4c80,0x8c41,
		0x4400,0x84c1,0x8581,0x4540,0x8701,0x47c0,0x4680,0x8641,0x8201,0x42c0,0x4380,0x8341,0x4100,0x81c1,0x8081,0x4040};

	uint16_t retValue=0;
	for (size_t i=0;i<len;i++)
		retValue=(retValue>>8)^CRCTable[(retValue&0xff)^buffer[i]];
	return retValue;
}

// this is really really quick 'n dirty
// leeway is suspicious. I can't see it from the official RNC ProPack, but seems to be present elsewhere...
void packRNC(Buffer &dest,const Buffer &source,uint32_t chunkSize)
{
	if (!chunkSize) chunkSize=32768;		// seems to be a good default
	if (chunkSize>65536) chunkSize=65536;
	if (chunkSize<4096) chunkSize=4096;

	std::vector<uint8_t> stream(20);
	stream[0]='R';
	stream[1]='N';
	stream[2]='C';
	stream[3]=1;

	stream[4]=uint8_t(source.size()>>24);
	stream[5]=uint8_t(source.size()>>16);
	stream[6]=uint8_t(source.size()>>8);
	stream[7]=uint8_t(source.size());

	stream[8]=stream[9]=stream[10]=stream[11]=0;	

	uint16_t rawCrc=RNCCRC(source.data(),source.size());
	stream[12]=uint8_t(rawCrc>>8);
	stream[13]=uint8_t(rawCrc);

	stream[14]=stream[15]=stream[16]=stream[17]=0;

	stream[18]=stream[19]=0;
	uint32_t bitStreamPosition=18;
	uint32_t bitAccumContent=0;
	uint32_t bitAccumCount=2;


	uint32_t offset=0;
	uint32_t chunkCount=0;
	uint32_t leeway=0;
	while (offset!=source.size())
	{
		auto bitLength=[](uint32_t value)->uint32_t
		{
			uint32_t ret=0;
			while (value)
			{
				value>>=1;
				ret++;
			}
			return ret;
		};

		// returns code, extra bits, value
		auto packValue=[&](uint32_t value)->std::tuple<uint32_t,uint32_t,uint32_t>
		{
			if (value<2)
				return std::tuple<uint32_t,uint32_t,uint32_t>{value,0,0};
			uint32_t bits=bitLength(value)-1;
			value&=(1<<bits)-1;
			return std::tuple<uint32_t,uint32_t,uint32_t>{bits+1,bits,value};
		};

		// bruteforce!!!
		// returns length,offset
		auto findLongestRepeat=[&](const uint8_t *buf,uint32_t offset,uint32_t length)->std::pair<uint32_t,uint32_t>
		{
			auto comparablePackedSize=[&](uint32_t value)->uint32_t
			{
				if (value<2) return 1;
				// fuzzy cost addition for longer codes
				return bitLength(value)+(bitLength(bitLength(value))<<1U);
			};

			std::pair<uint32_t,uint32_t> best{0,0};

			uint32_t distance=1;
			while (distance<=offset && distance<=chunkSize)
			{
				uint32_t i=0;
				while (offset+i<length && buf[offset+i]==buf[offset+i-distance])
					i++;
				if (i>=2 && (comparablePackedSize(i-2)+comparablePackedSize(distance-1)+best.first*8<comparablePackedSize(best.first-2)+comparablePackedSize(best.second-1)+i*8)
					// fuzzy cost addition for shorter blocks (extra literal turnaround)
					&& (comparablePackedSize(i-2)+comparablePackedSize(distance-1))+5<i*8 )
				{
					best=std::make_pair(i,distance);
				}
				distance+=1;
			}
			return best;
		};

		std::vector<uint32_t> litFrequencies(32,0);
		std::vector<uint32_t> distanceFrequencies(32,0);
		std::vector<uint32_t> lengthFrequencies(32,0);

		// this is what makes this implementaion even more bruteforce
		// table index (lit=0,distance=1,length=2,byte=3,bits=4), code, extra bits, value
		std::vector<std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>> rawChunk;
	
		uint32_t litCountOffset=0;
		bool litActive=false;

		// sub count
		rawChunk.push_back(std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>{4,0,16,0});

		auto packLit=[&]()
		{
			auto pack=packValue(std::get<3>(rawChunk[litCountOffset]));
			std::get<1>(rawChunk[litCountOffset])=std::get<0>(pack);
			std::get<2>(rawChunk[litCountOffset])=std::get<1>(pack);
			std::get<3>(rawChunk[litCountOffset])=std::get<2>(pack);
			litFrequencies[std::get<0>(pack)]+=1;
		};

		uint32_t foundLength=1;
		uint32_t currentChunkSize=std::min(uint32_t(source.size())-offset,chunkSize);
		for (uint32_t i=offset;i<currentChunkSize+offset;i+=foundLength)
		{
			auto fuzzyBreak=[&]()->bool
			{
				if (i+FUZZY_BLOCK_CUT_THRESHOLD>=currentChunkSize+offset)
				{
					currentChunkSize=i+foundLength-offset;
					return true;
				}
				return false;
			};

			if (!litActive)
			{
				// literal count
				litCountOffset=uint32_t(rawChunk.size());
				rawChunk.push_back(std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>{0,0,0,0});
				litActive=true;

				std::get<3>(rawChunk[0])+=1;
			}

			auto repeat=findLongestRepeat(source.data(),i,currentChunkSize+offset);
			if (repeat.first)
			{
				packLit();
				litActive=false;
				auto dist=packValue(repeat.second-1);
				rawChunk.push_back(std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>{1,std::get<0>(dist),std::get<1>(dist),std::get<2>(dist)});
				distanceFrequencies[std::get<0>(dist)]+=1;
				auto count=packValue(repeat.first-2);
				rawChunk.push_back(std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>{2,std::get<0>(count),std::get<1>(count),std::get<2>(count)});
				lengthFrequencies[std::get<0>(count)]+=1;
				foundLength=repeat.first;
			} else {
				rawChunk.push_back(std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>{3,source.data()[i],0,0});
				std::get<3>(rawChunk[litCountOffset])+=1;
				foundLength=1;
				if (/*std::get<3>(rawChunk[litCountOffset])==1 &&*/ fuzzyBreak()) break;
			}
		}
		if (litActive)
		{
			packLit();
		} else {
			litCountOffset=uint32_t(rawChunk.size());
			rawChunk.push_back(std::tuple<uint32_t,uint32_t,uint32_t,uint32_t>{0,0,0,0});
			packLit();

			std::get<3>(rawChunk[0])+=1;
		}
		offset+=currentChunkSize;
		chunkCount++;

		std::function<void(uint32_t,uint32_t)> writeBits=[&](uint32_t bitCount,uint32_t bits)
		{
			if (!bitCount) return;
			if (bitCount+bitAccumCount>16)
			{
				uint32_t bitsToWrite=16-bitAccumCount;
				writeBits(bitsToWrite,bits&((1<<bitsToWrite)-1));
				bits>>=bitsToWrite;
				bitCount-=bitsToWrite;
			}
			if (!bitAccumCount)
			{
				bitStreamPosition=uint32_t(stream.size());
				stream.push_back(0);
				stream.push_back(0);
			}
			bitAccumContent|=bits<<bitAccumCount;
			bitAccumCount+=bitCount;
			if (bitAccumCount==16)
			{
				stream[bitStreamPosition]=bitAccumContent;
				stream[bitStreamPosition+1]=bitAccumContent>>8;
				bitAccumContent=0;
				bitAccumCount=0;
			}
		};

		auto writeByte=[&](uint8_t byte)
		{
			stream.push_back(byte);
		};

		// also writes table to stream
		// result vector is code, length pairs
		auto createHuffmanCodeTable=[&](std::vector<std::pair<uint32_t,uint32_t>> &codes,const std::vector<uint32_t> &frequencies)
		{
			std::vector<std::pair<uint32_t,uint32_t>> sortedList;
			uint32_t totalCount=0;
			uint32_t totalFreq=0;
			for (uint32_t i=0;i<frequencies.size();i++)
			{
				totalFreq+=frequencies[i];
				if (frequencies[i])
				{
					totalCount=i+1;
					sortedList.push_back(std::make_pair(i,frequencies[i]));
				}
			}
			writeBits(5,totalCount);
			if (!totalCount) return;
			std::sort(sortedList.begin(),sortedList.end(),[&](const auto &a,const auto &b){return a.second>b.second||(a.second==b.second&&a.first<b.first);});

			// convert frequencies to bit length
			const uint32_t initialNorm=1<<30;
			uint32_t totalUsed=0;
			uint32_t sortedFrequencies[totalCount];
			for (uint32_t i=0;i<sortedList.size();i++)
			{
				uint32_t bitCount=1;	// extra +1 for later tuning
				uint32_t tmp=sortedList[i].second;
				sortedFrequencies[i]=tmp;
				while (tmp<totalFreq)
				{
					tmp<<=1;
					bitCount++;
				}
				sortedList[i].second=bitCount;
				totalUsed+=initialNorm>>bitCount;
			}

			// use the full range
			while (totalUsed!=initialNorm)
			{
				uint32_t bestIndex=totalCount;
				uint32_t bestImprovement=0;

				for (uint32_t i=0;i<sortedList.size();i++)
				{
					if (totalUsed+(initialNorm>>sortedList[i].second)<=initialNorm)
					{
						// adding cost factor here too
						uint32_t improvement=sortedFrequencies[i]<<sortedList[i].second;
						if (improvement>bestImprovement)
						{
							bestIndex=i;
							bestImprovement=improvement;
						}
					}
				}
				if (bestIndex==totalCount) break;
				totalUsed+=initialNorm>>sortedList[bestIndex].second;
				sortedList[bestIndex].second-=1;
			}

			// using the full range will sometimes result out-of-order indexes
			std::sort(sortedList.begin(),sortedList.end(),[&](const auto &a,const auto &b){return a.second<b.second||(a.second==b.second&&a.first<b.first);});

			uint32_t maxDepth=0;
			for (uint32_t i=0;i<sortedList.size();i++)
				maxDepth=std::max(maxDepth,sortedList[i].second);

			uint32_t value=0;
			for (uint32_t i=0;i<sortedList.size();i++)
			{
				auto reverseBits=[](uint32_t bitCount,uint32_t bits)->uint32_t
				{
					uint32_t ret=0;
					while (bitCount--)
					{
						ret<<=1;
						ret|=bits&1;
						bits>>=1;
					}
					return ret;
				};

				uint32_t code=sortedList[i].first;
				codes[code].first=reverseBits(maxDepth,value);
				codes[code].second=sortedList[i].second;
				value+=1<<(maxDepth-sortedList[i].second);
			}
			for (uint32_t i=0;i<totalCount;i++) writeBits(4,codes[i].second);
		};
		
		// code, length pairs
		std::vector<std::pair<uint32_t,uint32_t>> litCodes(32,std::make_pair(0,0));
		std::vector<std::pair<uint32_t,uint32_t>> distanceCodes(32,std::make_pair(0,0));
		std::vector<std::pair<uint32_t,uint32_t>> lengthCodes(32,std::make_pair(0,0));

		uint32_t streamStart=uint32_t(stream.size());
		createHuffmanCodeTable(litCodes,litFrequencies);
		createHuffmanCodeTable(distanceCodes,distanceFrequencies);
		createHuffmanCodeTable(lengthCodes,lengthFrequencies);

		for (uint32_t i=0;i<uint32_t(rawChunk.size());i++)
		{
			switch (std::get<0>(rawChunk[i]))
			{
				// literal
				case 0:
				writeBits(litCodes[std::get<1>(rawChunk[i])].second,litCodes[std::get<1>(rawChunk[i])].first);
				writeBits(std::get<2>(rawChunk[i]),std::get<3>(rawChunk[i]));
				break;

				// distance
				case 1:
				writeBits(distanceCodes[std::get<1>(rawChunk[i])].second,distanceCodes[std::get<1>(rawChunk[i])].first);
				writeBits(std::get<2>(rawChunk[i]),std::get<3>(rawChunk[i]));
				break;

				// length
				case 2:
				writeBits(lengthCodes[std::get<1>(rawChunk[i])].second,lengthCodes[std::get<1>(rawChunk[i])].first);
				writeBits(std::get<2>(rawChunk[i]),std::get<3>(rawChunk[i]));
				break;

				// bytes
				case 3:
				writeByte(std::get<1>(rawChunk[i]));
				break;

				// bits
				case 4:
				writeBits(std::get<2>(rawChunk[i]),std::get<3>(rawChunk[i]));
				break;

				default:
				break;
			}
		}
		uint32_t outputLength=uint32_t(stream.size())-streamStart;
		if (currentChunkSize>outputLength && outputLength-outputLength>leeway)
			leeway=outputLength-outputLength;
	}

	if (bitAccumCount)
	{
		stream[bitStreamPosition]=bitAccumContent;
		stream[bitStreamPosition+1]=bitAccumContent>>8;
		if (bitStreamPosition==stream.size()-2 && bitAccumCount<=8)
			stream.pop_back();
	}

	uint32_t packedSize=uint32_t(stream.size()-18);
	stream[8]=uint8_t(packedSize>>24);
	stream[9]=uint8_t(packedSize>>16);
	stream[10]=uint8_t(packedSize>>8);
	stream[11]=uint8_t(packedSize);

	uint16_t packedCrc=RNCCRC(stream.data()+18,packedSize);
	stream[14]=uint8_t(packedCrc>>8);
	stream[15]=uint8_t(packedCrc);

	if (leeway>255)
	{
		fprintf(stderr,"Leeway larger than 255\n");
		exit(-1);
	}

	stream[16]=leeway;
	stream[17]=chunkCount;

	dest.resize(stream.size());
	std::memcpy(dest.data(),stream.data(),stream.size());
}

int main(int argc,char **argv)
{
	auto usage=[]()
	{
		fprintf(stderr,"Usage: <prog> input_raw output_packed [chunk_size]\n");
	};

	if (argc<3)
	{
		usage();
		return -1;
	}

	auto raw{readFile(argv[1])};

	VectorBuffer packed;
	packRNC(packed,*raw,(argc>=4)?atoi(argv[3]):0);

	writeFile(argv[2],packed);
	return 0;
}

}

int main(int argc,char **argv)
{
	return ancient::internal::main(argc,argv);
}
