/* Copyright (C) Teemu Suutari */

#include <stdint.h>
#include <string.h>

#include "BZIP2Decompressor.hpp"
#include "HuffmanDecoder.hpp"

#include <FixedMemoryBuffer.hpp>
#include <CRC32.hpp>

bool BZIP2Decompressor::detectHeader(uint32_t hdr) noexcept
{
	return ((hdr&0xffff'ff00U)==FourCC('BZh\0') && (hdr&0xffU)>='1' && (hdr&0xffU)<='9');
}

bool BZIP2Decompressor::detectHeaderXPK(uint32_t hdr) noexcept
{
	return (hdr==FourCC('BZP2'));
}

std::unique_ptr<Decompressor> BZIP2Decompressor::create(const Buffer &packedData,bool exactSizeKnown,bool verify)
{
	return std::make_unique<BZIP2Decompressor>(packedData,exactSizeKnown,verify);
}

std::unique_ptr<XPKDecompressor> BZIP2Decompressor::create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify)
{
	return std::make_unique<BZIP2Decompressor>(hdr,recursionLevel,packedData,state,verify);
}

BZIP2Decompressor::BZIP2Decompressor(const Buffer &packedData,bool exactSizeKnown,bool verify) :
	_packedData(packedData),
	_packedSize(0)
{
	uint32_t hdr=packedData.readBE32(0);
	if (!detectHeader(hdr)) throw Decompressor::InvalidFormatError();;
	_blockSize=((hdr&0xffU)-'0')*100'000;
}

BZIP2Decompressor::BZIP2Decompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify) :
	XPKDecompressor(recursionLevel),
	_packedData(packedData),
	_packedSize(_packedData.size())
{
	uint32_t blockHdr=packedData.readBE32(0);
	if (!detectHeader(blockHdr)) throw Decompressor::InvalidFormatError();;
	_blockSize=((blockHdr&0xffU)-'0')*100'000;
}

BZIP2Decompressor::~BZIP2Decompressor()
{
	// nothing needed
}

const std::string &BZIP2Decompressor::getName() const noexcept
{
	static std::string name="bz2: bzip2";
	return name;
}

const std::string &BZIP2Decompressor::getSubName() const noexcept
{
	static std::string name="XPK-BZP2: bzip2";
	return name;
}

size_t BZIP2Decompressor::getPackedSize() const noexcept
{
	// no way to know before decompressing
	return _packedSize;
}


size_t BZIP2Decompressor::getRawSize() const noexcept
{
	// same thing, decompression needed first
	return _rawSize;
}

void BZIP2Decompressor::decompressImpl(Buffer &rawData,bool verify)
{
	size_t packedSize=_packedSize?_packedSize:_packedData.size();
	size_t rawSize=_rawSize?_rawSize:rawData.size();

	const uint8_t *bufPtr=_packedData.data();
	size_t bufOffset=4;

	uint8_t bufBitsLength=0;
	uint32_t bufBitsContent=0;

	// stream verification
	//
	// there is so much wrong in bzip2 CRC-calculation :(
	// 1. The bit ordering is opposite what everyone else does with CRC32
	// 2. The block CRCs are calculated separately, no way of calculating a complete
	//    CRC without knowing the block layout
	// 3. The CRC is the end of the stream and the stream is bit aligned. You
	//    can't read CRC without decompressing the stream.
	uint32_t crc=0;
	auto calculateBlockCRC=[&](size_t blockPos,size_t blockSize)
	{
		crc=(crc<<1)|(crc>>31);
		crc^=CRC32Rev(rawData,blockPos,blockSize,0);
	};

	// streamreader
	auto readBits=[&](uint32_t count)->uint32_t
	{
		auto readBitsInt=[&](uint32_t count)->uint32_t
		{
			while (bufBitsLength<count)
			{
				if (bufOffset>=packedSize) throw DecompressionError();
				bufBitsContent=(bufBitsContent<<8)|bufPtr[bufOffset++];
				bufBitsLength+=8;
			}
			uint32_t ret=(bufBitsContent>>(bufBitsLength-count))&((1<<count)-1);
			bufBitsLength-=count;
			return ret;
		};

		// avoiding use of 64 bit math here
		uint32_t ret;
		if (count>24)
		{
			ret=readBitsInt(16)<<(count-16);
			ret|=readBitsInt(count-16);
		} else ret=readBitsInt(count);
		return ret;
	};

	auto readBit=[&]()->uint8_t
	{
		return readBits(1);
	};
	
	uint8_t *dest=rawData.data();
	size_t destOffset=0;

	HuffmanDecoder<uint8_t,0xffU,6> selectorDecoder
	{
		// incomplete Huffman table. errors possible
		HuffmanCode<uint8_t>{1,0b000000,0},
		HuffmanCode<uint8_t>{2,0b000010,1},
		HuffmanCode<uint8_t>{3,0b000110,2},
		HuffmanCode<uint8_t>{4,0b001110,3},
		HuffmanCode<uint8_t>{5,0b011110,4},
		HuffmanCode<uint8_t>{6,0b111110,5}
	};

	HuffmanDecoder<int32_t,2,2> deltaDecoder
	{
		HuffmanCode<int32_t>{1,0b00,0},
		HuffmanCode<int32_t>{2,0b10,1},
		HuffmanCode<int32_t>{2,0b11,-1}
	};

	FixedMemoryBuffer tmpBuffer(_blockSize);
	uint8_t *tmpBufferPtr=tmpBuffer.data();

	// This is the dark, ancient secret of bzip2.
	// versions before 0.9.5 had a data randomization for "too regular"
	// data problematic for the bwt-implementation at that time.
	// although it is never utilized anymore, the support is still there
	// And this is exactly the kind of ancient stuff we want to support :)
	//
	// On this specific part (since it is a table of magic numbers)
	// we have no way other than copying it from the original reference
	// thus for this table (and this table only), the license is as:

/*
--------------------------------------------------------------------------

This program, "bzip2", the associated library "libbzip2", and all
documentation, are copyright (C) 1996-2010 Julian R Seward.  All
rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. The origin of this software must not be misrepresented; you must 
   not claim that you wrote the original software.  If you use this 
   software in a product, an acknowledgment in the product 
   documentation would be appreciated but is not required.

3. Altered source versions must be plainly marked as such, and must
   not be misrepresented as being the original software.

4. The name of the author may not be used to endorse or promote 
   products derived from this software without specific prior written 
   permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Julian Seward, jseward@bzip.org
bzip2/libbzip2 version 1.0.6 of 6 September 2010

--------------------------------------------------------------------------
*/

// content formatted, numbers -1 (i.e. this is altered version by clause 3)
	static const uint16_t randomTable[512]={
		619-1,720-1,127-1,481-1,931-1,816-1,813-1,233-1,566-1,247-1,985-1,724-1,205-1,454-1,863-1,491-1,
		741-1,242-1,949-1,214-1,733-1,859-1,335-1,708-1,621-1,574-1, 73-1,654-1,730-1,472-1,419-1,436-1,
		278-1,496-1,867-1,210-1,399-1,680-1,480-1, 51-1,878-1,465-1,811-1,169-1,869-1,675-1,611-1,697-1,
		867-1,561-1,862-1,687-1,507-1,283-1,482-1,129-1,807-1,591-1,733-1,623-1,150-1,238-1, 59-1,379-1,
		684-1,877-1,625-1,169-1,643-1,105-1,170-1,607-1,520-1,932-1,727-1,476-1,693-1,425-1,174-1,647-1,
		 73-1,122-1,335-1,530-1,442-1,853-1,695-1,249-1,445-1,515-1,909-1,545-1,703-1,919-1,874-1,474-1,
		882-1,500-1,594-1,612-1,641-1,801-1,220-1,162-1,819-1,984-1,589-1,513-1,495-1,799-1,161-1,604-1,
		958-1,533-1,221-1,400-1,386-1,867-1,600-1,782-1,382-1,596-1,414-1,171-1,516-1,375-1,682-1,485-1,
		911-1,276-1, 98-1,553-1,163-1,354-1,666-1,933-1,424-1,341-1,533-1,870-1,227-1,730-1,475-1,186-1,
		263-1,647-1,537-1,686-1,600-1,224-1,469-1, 68-1,770-1,919-1,190-1,373-1,294-1,822-1,808-1,206-1,
		184-1,943-1,795-1,384-1,383-1,461-1,404-1,758-1,839-1,887-1,715-1, 67-1,618-1,276-1,204-1,918-1,
		873-1,777-1,604-1,560-1,951-1,160-1,578-1,722-1, 79-1,804-1, 96-1,409-1,713-1,940-1,652-1,934-1,
		970-1,447-1,318-1,353-1,859-1,672-1,112-1,785-1,645-1,863-1,803-1,350-1,139-1, 93-1,354-1, 99-1,
		820-1,908-1,609-1,772-1,154-1,274-1,580-1,184-1, 79-1,626-1,630-1,742-1,653-1,282-1,762-1,623-1,
		680-1, 81-1,927-1,626-1,789-1,125-1,411-1,521-1,938-1,300-1,821-1, 78-1,343-1,175-1,128-1,250-1,
		170-1,774-1,972-1,275-1,999-1,639-1,495-1, 78-1,352-1,126-1,857-1,956-1,358-1,619-1,580-1,124-1,
		737-1,594-1,701-1,612-1,669-1,112-1,134-1,694-1,363-1,992-1,809-1,743-1,168-1,974-1,944-1,375-1,
		748-1, 52-1,600-1,747-1,642-1,182-1,862-1, 81-1,344-1,805-1,988-1,739-1,511-1,655-1,814-1,334-1,
		249-1,515-1,897-1,955-1,664-1,981-1,649-1,113-1,974-1,459-1,893-1,228-1,433-1,837-1,553-1,268-1,
		926-1,240-1,102-1,654-1,459-1, 51-1,686-1,754-1,806-1,760-1,493-1,403-1,415-1,394-1,687-1,700-1,
		946-1,670-1,656-1,610-1,738-1,392-1,760-1,799-1,887-1,653-1,978-1,321-1,576-1,617-1,626-1,502-1,
		894-1,679-1,243-1,440-1,680-1,879-1,194-1,572-1,640-1,724-1,926-1, 56-1,204-1,700-1,707-1,151-1,
		457-1,449-1,797-1,195-1,791-1,558-1,945-1,679-1,297-1, 59-1, 87-1,824-1,713-1,663-1,412-1,693-1,
		342-1,606-1,134-1,108-1,571-1,364-1,631-1,212-1,174-1,643-1,304-1,329-1,343-1, 97-1,430-1,751-1,
		497-1,314-1,983-1,374-1,822-1,928-1,140-1,206-1, 73-1,263-1,980-1,736-1,876-1,478-1,430-1,305-1,
		170-1,514-1,364-1,692-1,829-1, 82-1,855-1,953-1,676-1,246-1,369-1,970-1,294-1,750-1,807-1,827-1,
		150-1,790-1,288-1,923-1,804-1,378-1,215-1,828-1,592-1,281-1,565-1,555-1,710-1, 82-1,896-1,831-1,
		547-1,261-1,524-1,462-1,293-1,465-1,502-1, 56-1,661-1,821-1,976-1,991-1,658-1,869-1,905-1,758-1,
		745-1,193-1,768-1,550-1,608-1,933-1,378-1,286-1,215-1,979-1,792-1,961-1, 61-1,688-1,793-1,644-1,
		986-1,403-1,106-1,366-1,905-1,644-1,372-1,567-1,466-1,434-1,645-1,210-1,389-1,550-1,919-1,135-1,
		780-1,773-1,635-1,389-1,707-1,100-1,626-1,958-1,165-1,504-1,920-1,176-1,193-1,713-1,857-1,265-1,
		203-1, 50-1,668-1,108-1,645-1,990-1,626-1,197-1,510-1,357-1,358-1,850-1,858-1,364-1,936-1,638-1};

// end the table, back to the usual license & copyright

	for (;;)
	{
		uint32_t blockHdrHigh=readBits(32);
		uint32_t blockHdrLow=readBits(16);
		if (blockHdrHigh==0x31415926U && blockHdrLow==0x5359U)
		{
			// a block

			// this is rather spaghetti...
			readBits(32);	// block crc, not interested
			bool randomized=readBit();

			// basically the random inserted is one LSB after n-th bytes
			// per defined in the table.
			uint32_t randomPos=1;
			uint32_t randomCounter=randomTable[0]-1;
			auto randomBit=[&]()->bool
			{
				// Beauty is in the eye of the beholder: this is smallest form to hide the ugliness
				return (!randomCounter--)?randomCounter=randomTable[randomPos++&511]:false;
			};

			uint32_t currentPtr=readBits(24);

			uint32_t currentBlockSize=0;
			{
				uint32_t numHuffmanItems=2;
				uint32_t huffmanValues[256];

				{
					// this is just a little bit inefficient but still we reading bit by bit since
					// reference does it. (bitsream format details do not spill over)
					std::vector<bool> usedMap(16);
					for (uint32_t i=0;i<16;i++) usedMap[i]=readBit();

					std::vector<bool> huffmanMap(256);
					for (uint32_t i=0;i<16;i++)
					{
						for (uint32_t j=0;j<16;j++)
							huffmanMap[i*16+j]=(usedMap[i])?readBit():false;
					}

					for (uint32_t i=0;i<256;i++) if (huffmanMap[i]) numHuffmanItems++;
					if (numHuffmanItems==2) throw DecompressionError();

					for (uint32_t currentValue=0,i=0;i<256;i++)
						if (huffmanMap[i]) huffmanValues[currentValue++]=i;
				}

				uint32_t huffmanGroups=readBits(3);
				if (huffmanGroups<2 || huffmanGroups>6) throw DecompressionError();

				uint32_t selectorsUsed=readBits(15);
				if (!selectorsUsed) throw DecompressionError();

				FixedMemoryBuffer huffmanSelectorList(selectorsUsed);

				auto unMFT=[](uint8_t value,uint8_t map[])->uint8_t
				{
					uint8_t ret=map[value];
					if (value)
					{
						uint8_t tmp=map[value];
						for (uint32_t i=value;i;i--)
							map[i]=map[i-1];
						map[0]=tmp;
					}
					return ret;
				};

				// create Huffman selectors
				uint8_t selectorMFTMap[6]={0,1,2,3,4,5};

				for (uint32_t i=0;i<selectorsUsed;i++)
				{
					uint8_t item=unMFT(selectorDecoder.decode(readBit),selectorMFTMap);
					if (item>=huffmanGroups) throw DecompressionError();
					huffmanSelectorList[i]=item;
				}

				typedef HuffmanDecoder<uint32_t,258,0> BZIP2Decoder;
				std::vector<BZIP2Decoder> dataDecoders(huffmanGroups);

				// Create all tables
				for (uint32_t i=0;i<huffmanGroups;i++)
				{
					uint8_t bitLengths[numHuffmanItems];

					uint32_t currentBits=readBits(5);
					for (uint32_t j=0;j<numHuffmanItems;j++)
					{
						int32_t delta;
						do
						{
							delta=deltaDecoder.decode(readBit);
							currentBits+=delta;
						} while (delta);
						if (currentBits<1 || currentBits>20) throw DecompressionError();
						bitLengths[j]=currentBits;
					}

					CreateOrderlyHuffmanTable(dataDecoders[i],bitLengths,numHuffmanItems);
				}

				// de-Huffman + unRLE + deMFT
				BZIP2Decoder *currentHuffmanDecoder=nullptr;
				uint32_t currentHuffmanIndex=0;
				uint8_t dataMFTMap[256];
				for (uint32_t i=0;i<numHuffmanItems-2;i++) dataMFTMap[i]=i;

				uint32_t currentRunLength=0;
				uint32_t currentRLEWeight=1;

				auto decodeRLE=[&]()
				{
					if (currentRunLength)
					{
						if (currentBlockSize+currentRunLength>_blockSize) throw DecompressionError();
						for (uint32_t i=0;i<currentRunLength;i++) tmpBufferPtr[currentBlockSize++]=huffmanValues[dataMFTMap[0]];
					}
					currentRunLength=0;
					currentRLEWeight=1;
				};

				for (uint32_t streamIndex=0;;streamIndex++)
				{
					if (!(streamIndex%50))
					{
						if (currentHuffmanIndex>=selectorsUsed) throw DecompressionError();
						currentHuffmanDecoder=&dataDecoders[huffmanSelectorList[currentHuffmanIndex++]];
					}
					uint32_t symbolMFT=currentHuffmanDecoder->decode(readBit);
					// stop marker is referenced only once, and it is the last one
					// This means we do no have to un-MFT it for detection
					if (symbolMFT==numHuffmanItems-1) break;
					if (currentBlockSize>=_blockSize) throw DecompressionError();
					if (symbolMFT<2)
					{
						currentRunLength+=currentRLEWeight<<symbolMFT;
						currentRLEWeight<<=1;
					} else {
						decodeRLE();
						uint8_t symbol=unMFT(symbolMFT-1,dataMFTMap);
						if (currentBlockSize>=_blockSize) throw DecompressionError();
						tmpBufferPtr[currentBlockSize++]=huffmanValues[symbol];
					}
				}
				decodeRLE();
				if (currentPtr>=currentBlockSize) throw DecompressionError();
			}

			// inverse BWT + final RLE decoding.
			// there are a few dark corners here as well
			// 1. Can the stream end at 4 literals without count? I assume it is a valid optimization (and that this does not spillover to next block)
			// 2. Can the RLE-step include counts 252 to 255 even if reference does not do them? I assume yes here as here as well
			// 3. Can the stream be empty? We do not take issue here about that (that should be culled out earlier already)
			uint32_t sums[256];
			for (uint32_t i=0;i<256;i++) sums[i]=0;

			for (uint32_t i=0;i<currentBlockSize;i++)
			{
				sums[tmpBufferPtr[i]]++;
			}

			uint32_t rank[256];
			for (uint32_t tot=0,i=0;i<256;i++)
			{
				rank[i]=tot;
				tot+=sums[i];
			}

			// not at all happy about the memory consumption, but it simplifies the implementation a lot
			// and by sacrificing 4*size (size as in actual block size) we do not have to have slow search nor another temporary buffer
			// since by calculating forward table we can do forward decoding of the data on the same pass as iBWT
			//
			// also, because I'm lazy
			FixedMemoryBuffer forwardIndex(currentBlockSize*sizeof(uint32_t));
			uint32_t *forwardIndexPtr=reinterpret_cast<uint32_t*>(forwardIndex.data());
			for (uint32_t i=0;i<currentBlockSize;i++)
				forwardIndexPtr[rank[tmpBufferPtr[i]]++]=i;

			// output + final RLE decoding
			uint8_t currentCh=0;
			uint32_t currentChCount=0;
			auto outputByte=[&](uint8_t ch)
			{
				if (randomized && randomBit()) ch^=1;
				if (!currentChCount)
				{
					currentCh=ch;
					currentChCount=1;
				} else {
					if (ch==currentCh && currentChCount!=4)
					{
						currentChCount++;
					} else {
						auto outputBlock=[&](uint32_t count)
						{
							if (destOffset+count>rawSize) throw DecompressionError();
							for (uint32_t i=0;i<count;i++) dest[destOffset++]=currentCh;
						};

						if (currentChCount==4)
						{
							outputBlock(uint32_t(ch)+4);
							currentChCount=0;
						} else {
							outputBlock(currentChCount);
							currentCh=ch;
							currentChCount=1;
						}
					}
				}
			};

			size_t destOffsetStart=destOffset;

			// and now the final iBWT + unRLE is easy...
			for (uint32_t i=0;i<currentBlockSize;i++)
			{
				currentPtr=forwardIndexPtr[currentPtr];
				outputByte(tmpBufferPtr[currentPtr]);
			}
			// cleanup the state, a bit hackish way to do it
			if (currentChCount) outputByte(currentChCount==4?0:~currentCh);

			if (verify)
				calculateBlockCRC(destOffsetStart,destOffset-destOffsetStart);

		} else if (blockHdrHigh==0x17724538U && blockHdrLow==0x5090U) {
			// end of blocks
			uint32_t rawCRC=readBits(32);
			if (verify && crc!=rawCRC) throw VerificationError();
			break;
		} else throw DecompressionError();
	}

	if (!_rawSize) _rawSize=destOffset;
	if (!_packedSize) _packedSize=bufOffset;
	if (_rawSize!=destOffset) throw DecompressionError();
}

void BZIP2Decompressor::decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify)
{
	return decompressImpl(rawData,verify);
}

Decompressor::Registry<BZIP2Decompressor> BZIP2Decompressor::_registration;
XPKDecompressor::Registry<BZIP2Decompressor> BZIP2Decompressor::_XPKregistration;
