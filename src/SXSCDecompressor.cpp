/* Copyright (C) Teemu Suutari */

#include "SXSCDecompressor.hpp"

#include "InputStream.hpp"
#include "OutputStream.hpp"
#include "DLTADecode.hpp"
#include "common/MemoryBuffer.hpp"

// not very pretty arithcoder, but arithcoders
// are very sensitive to implementation details
// thus we have to implement it just like original
SXSCDecompressor::ArithDecoder::ArithDecoder(ForwardInputStream &inputStream) :
	_bitReader(inputStream)
{
	_stream=inputStream.readByte()<<8;
	_stream|=inputStream.readByte();
}

SXSCDecompressor::ArithDecoder::~ArithDecoder()
{
	// nothing needed
}

uint16_t SXSCDecompressor::ArithDecoder::decode(uint16_t length)
{
	return ((uint32_t(_stream-_low)+1)*length-1)/(uint32_t(_high-_low)+1);
}

void SXSCDecompressor::ArithDecoder::scale(uint16_t newLow,uint16_t newHigh,uint16_t newRange)
{
	auto readBit=[&]()->uint32_t
	{
		return _bitReader.readBits8(1);
	};

	uint32_t range=uint32_t(_high-_low)+1;
	_high=(range*newHigh)/newRange+_low-1;
	_low=(range*newLow)/newRange+_low;
	while (!((_low^_high)&0x8000U))
	{
		_low<<=1;
		_high=(_high<<1)|1U;
		_stream=(_stream<<1)|readBit();
	}
	// funky!
	while ((_low&0x4000U)&&!(_high&0x4000U))
	{
		_low=(_low<<1)&0x7fffU;
		_high=(_high<<1)|0x8001U;
		_stream=((_stream<<1)^0x8000U)|readBit();
	}
}


bool SXSCDecompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return hdr==FourCC('SASC');//||hdr==FourCC('SHSC');
}

std::unique_ptr<XPKDecompressor> SXSCDecompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<SXSCDecompressor>(hdr,recursionLevel,packedData,state,verify);
}

SXSCDecompressor::SXSCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData),
	_isHSC(hdr==FourCC('SHSC'))
{
	if (!detectHeaderXPK(hdr)) throw Decompressor::InvalidFormatError();
}

SXSCDecompressor::~SXSCDecompressor()
{
	// nothing needed
}

const std::string &SXSCDecompressor::getSubName() const noexcept
{
	static std::string nameASC="XPK-SASC: LZ-compressor with arithmetic and delta encoding";
	static std::string nameHSC="XPK-SHSC: Context modeling compressor";
	return _isHSC?nameHSC:nameASC;
}

void SXSCDecompressor::decompressASC(Buffer &rawData,ForwardInputStream &inputStream)
{
	ForwardOutputStream outputStream(rawData,0,rawData.size());
	ArithDecoder arithDecoder(inputStream);

	// decoder for literal, copy, end decision
	// two thresholds -> 3 symbols, last symbol is break with size of 1
	uint16_t bitThreshold1[4]={40,40,40,40};
	uint16_t bitThreshold2[4]={40,40,40,40};
	uint32_t bitPos=0;

	// generics for the other decoder

	auto tableElements=[](auto &table)->uint16_t
	{
		return (sizeof(table)/sizeof(*table)+1)>>1;
	};

	auto initTable=[](auto &table,uint16_t initialValue)
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		for (uint32_t i=0;i<length;i++) table[i]=initialValue;
		for (uint32_t j=0,i=length;i<length*2-1;i++,j+=2)
			table[i]=table[j]+table[j+1];
	};

	auto updateTable=[](auto &table,uint16_t max,uint16_t index,int16_t value)
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		for (uint32_t i=index;i<length*2-1;i=(i>>1)+length)
			table[i]+=value;
		if (table[length*2-2]>=max)
		{
			for (uint32_t i=0;i<length;i++)
				if (table[i]>1) table[i]>>=1;
			for (uint32_t j=0,i=length;i<length*2-1;i++,j+=2)
				table[i]=table[j]+table[j+1];
		}
	};

	auto tableSize=[](auto &table)->uint16_t
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		return table[length*2-2];
	};

	auto decodeSymbol=[](auto &table,uint16_t &value)->uint16_t
	{
		constexpr uint32_t length=(sizeof(table)/sizeof(*table)+1)>>1;
		uint32_t threshold=0;
		uint32_t i=length*2-4;
		while (i>=length)
		{
			uint32_t child=(i-length)<<1;
			if (value-threshold>=table[i])
			{
				threshold+=table[i];
				child+=2;
			}
			i=child;
		}
		if (value-threshold>=table[i])
		{
			threshold+=table[i];
			i++;
		}
		value=threshold;
		return i;
	};

	// literal decoder
	uint16_t litInitial[256*2-1];
	uint16_t litDynamic[256*2-1];
	uint16_t litThreshold=1;

	initTable(litInitial,1);
	initTable(litDynamic,0);

	// distance / length decoder
	uint16_t distanceCodes[16*2-1];
	uint16_t countInitial[64*2-1];
	uint16_t countDynamic[64*2-1];
	uint16_t countThreshold=8;

	initTable(distanceCodes,0);
	initTable(countInitial,1);
	initTable(countDynamic,0);

	updateTable(distanceCodes,6000,0,24);
	uint32_t distanceIndex=0;

	auto twoStepArithDecoder=[&](auto &initialTable,auto &dynamicTable,uint16_t &threshold,uint16_t max,uint16_t step,uint16_t updateRange)->uint16_t
	{
		uint16_t value=arithDecoder.decode(tableSize(dynamicTable)+threshold);
		uint16_t ret;
		if (value<tableSize(dynamicTable))
		{
			ret=decodeSymbol(dynamicTable,value);
			arithDecoder.scale(value,value+dynamicTable[ret],tableSize(dynamicTable)+threshold);
		} else {
			arithDecoder.scale(tableSize(dynamicTable),tableSize(dynamicTable)+threshold,tableSize(dynamicTable)+threshold);

			value=arithDecoder.decode(tableSize(initialTable));
			ret=decodeSymbol(initialTable,value);
			arithDecoder.scale(value,value+initialTable[ret],tableSize(initialTable));
			updateTable(initialTable,65535,ret,-initialTable[ret]);
			if (tableSize(initialTable)) threshold+=step;
				else threshold=0;
			for (uint32_t i=ret>updateRange?ret-updateRange:0;i<std::min(uint16_t(ret+updateRange),uint16_t(tableElements(initialTable)-1U));i++)
				if (initialTable[i]) updateTable(initialTable,max,i,1);
		}
		updateTable(dynamicTable,max,ret,step);
		if (dynamicTable[ret]==step*3) threshold=threshold>step?threshold-step:1;
		return ret;
	};

	for (;;)
	{
		uint16_t bitSize=bitThreshold1[bitPos]+bitThreshold2[bitPos];
		uint16_t bitValue=arithDecoder.decode(bitSize+1);
		if (bitValue==bitSize) break;
		bool bit=bitValue<bitThreshold1[bitPos];
		arithDecoder.scale(bit?0:bitThreshold1[bitPos],bit?bitThreshold1[bitPos]:bitSize,bitSize+1);
		(bit?bitThreshold1:bitThreshold2)[bitPos]+=40;
		if (bitSize>=6000)
		{
			if (!(bitThreshold1[bitPos]>>=1)) bitThreshold1[bitPos]=1;
			if (!(bitThreshold2[bitPos]>>=1)) bitThreshold2[bitPos]=1;
		}
		bitPos=(bitPos<<1&2)|(bit?0:1);
		if (bit)
		{
			// literal
			outputStream.writeByte(twoStepArithDecoder(litInitial,litDynamic,litThreshold,1000,1,8));
		} else {
			// copy
			while (outputStream.getOffset()>(1<<distanceIndex) && distanceIndex<15)
				updateTable(distanceCodes,6000,++distanceIndex,24);
			uint16_t distanceValue=arithDecoder.decode(tableSize(distanceCodes));
			uint16_t distanceBits=decodeSymbol(distanceCodes,distanceValue);
			arithDecoder.scale(distanceValue,distanceValue+distanceCodes[distanceBits],tableSize(distanceCodes));
			updateTable(distanceCodes,6000,distanceBits,24);
			uint32_t distance=distanceBits;
			if (distanceBits>=2)
			{
				uint16_t minRange=1<<(distanceBits-1);
				uint16_t range=distanceIndex==distanceBits?std::min(outputStream.getOffset(),size_t(31200U))-minRange:minRange;
				distance=arithDecoder.decode(range);
				arithDecoder.scale(distance,distance+1,range);
				distance+=minRange;
			}
			distance++;
			uint32_t count=twoStepArithDecoder(countInitial,countDynamic,countThreshold,6000,8,4);
			if (count==15)
			{
				count=783;
			} else if (count>=16) {
				uint16_t value=arithDecoder.decode(16);
				arithDecoder.scale(value,value+1,16);
				count=((count-16)<<4)+value+15;
			}
			count+=3;
			outputStream.copy(distance,count);
		}
	}
	if (!outputStream.eof()) throw Decompressor::DecompressionError();
}

void SXSCDecompressor::decompressHSC(Buffer &rawData,ForwardInputStream &inputStream)
{
	// todo
}

void SXSCDecompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	ForwardInputStream inputStream(_packedData,0,_packedData.size(),true);

	uint8_t mode=inputStream.readByte();

	bool needsTmpBuffer=(mode>=2);
	std::unique_ptr<MemoryBuffer> tmpBuffer;
	if (needsTmpBuffer) tmpBuffer=std::make_unique<MemoryBuffer>(rawData.size());
	if (_isHSC) decompressHSC(needsTmpBuffer?*tmpBuffer:rawData,inputStream);
		else decompressASC(needsTmpBuffer?*tmpBuffer:rawData,inputStream);

	// Mono, High byte only
	// also includes de-interleaving
	auto deltaDecode16BE=[&]()
	{
		size_t length=rawData.size();
		const uint8_t *src=tmpBuffer->data();
		const uint8_t *midSrc=&src[length>>1];
		uint8_t *dest=rawData.data();

		uint8_t ctr=0;
		for (size_t i=0,j=0;j<length;i++,j+=2)
		{
			ctr+=src[i];
			dest[j]=ctr;
			dest[j+1]=midSrc[i];
		}
		if (length&1) dest[length-1]=src[length-1];
	};

	// Stereo, High byte only
	// also includes de-interleaving
	auto deltaDecode16LE=[&]()
	{
		size_t length=rawData.size();
		const uint8_t *src=tmpBuffer->data();
		const uint8_t *midSrc=&src[length>>1];
		uint8_t *dest=rawData.data();

		uint8_t ctr=0;
		for (size_t i=0,j=0;j<length;i++,j+=2)
		{
			dest[j]=midSrc[i];
			ctr+=src[i];
			dest[j+1]=ctr;
		}
		if (length&1) dest[length-1]=src[length-1];
	};

	switch (mode)
	{
		case 0:
		// no delta
		break;

		case 1:
		DLTADecode::decode(rawData,rawData,0,rawData.size());
		break;
		
		case 2:
		deltaDecode16BE();
		break;

		case 3:
		deltaDecode16LE();
		break;

		default:
		throw Decompressor::DecompressionError();
	}
}

XPKDecompressor::Registry<SXSCDecompressor> SXSCDecompressor::_XPKregistration;
