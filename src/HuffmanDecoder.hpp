/* Copyright (C) Teemu Suutari */

#ifndef HUFFMANDECODER_HPP
#define HUFFMANDECODER_HPP

#include <stddef.h>
#include <stdint.h>

#include <vector>

// For exception
#include "Decompressor.hpp"

template<typename T>
struct HuffmanCode
{
	uint32_t	length;
	size_t		code;

	T		value;
};

template<typename T,T emptyValue,size_t depth>
class HuffmanDecoder
{
private:
	static const size_t _length=(2<<depth)-2;

public:
	typedef T ItemType;
	typedef HuffmanCode<T> CodeType;

	HuffmanDecoder(const HuffmanDecoder&)=delete;
	HuffmanDecoder& operator=(const HuffmanDecoder&)=delete;

	HuffmanDecoder()
	{
		for (size_t i=0;i<_length;i++) _table[i]=emptyValue;
	}

	template<typename ...Args>
	HuffmanDecoder(const Args&& ...args) :
		HuffmanDecoder()
	{
		const HuffmanCode<T> list[sizeof...(args)]={args...};
		for (auto &item : list)
			insert(item);
	}

	~HuffmanDecoder()
	{
	}

	void reset()
	{
		for (size_t i=0;i<_length;i++) _table[i]=emptyValue;
	}

	template<typename F>
	T decode(F bitReader) const
	{
		size_t i=0;
		T ret;
		do {
			if (bitReader()) i++;
			ret=_table[i];
			i=i*2+2;
		} while (ret==emptyValue && i<_length);
		if (ret==emptyValue) throw Decompressor::DecompressionError();
		return ret;
	}

	void insert(const HuffmanCode<T> &code)
	{
		if (code.value==emptyValue || code.length>depth) throw Decompressor::DecompressionError();
		for (size_t i=0,j=code.length;j!=0;j--)
		{
			if (code.code&(1<<(j-1))) i++;
			if (_table[i]!=emptyValue) throw Decompressor::DecompressionError();
			if (j==1) _table[i]=code.value;
			i=i*2+2;
		}
	}

private:
	T	_table[_length];
};

// better for non-fixed sizes (especially deep ones)

template<typename T,T emptyValue>
class HuffmanDecoder<T,emptyValue,0>
{
	template<typename U> friend void CreateOrderlyHuffmanTable(U &dec,const uint8_t *bitLengths,uint32_t bitTableLength);

private:
	struct Node
	{
		uint32_t	sub[2];
		T		value;

		Node(uint32_t _sub0,uint32_t _sub1,T _value) :
			sub{_sub0,_sub1},
			value(_value)
		{
			// nothing needed
		}

		Node(Node &&source) :
			sub{source.sub[0],source.sub[1]},
			value(source.value)
		{
			// nothing needed
		}

		Node& operator=(Node &&source)
		{
			if (this!=&source)
			{
				sub[0]=source.sub[0];
				sub[1]=source.sub[1];
				value=source.value;
			}
			return *this;
		}
	};

public:
	typedef T ItemType;
	typedef HuffmanCode<T> CodeType;

	HuffmanDecoder()
	{
		_table.emplace_back(0,0,emptyValue);
	}

	template<typename ...Args>
	HuffmanDecoder(const Args&& ...args) :
		HuffmanDecoder()
	{
		const HuffmanCode<T> list[sizeof...(args)]={args...};
		for (auto &item : list)
			insert(item);
	}

	~HuffmanDecoder()
	{
	}

	void reset()
	{
		_table.clear();
		_table.emplace_back(0,0,emptyValue);
	}

	template<typename F>
	T decode(F bitReader) const
	{
		uint32_t length=uint32_t(_table.size());
		T ret=emptyValue;
		uint32_t i=0;
		while (i<length && ret==emptyValue)
		{
			i=_table[i].sub[bitReader()?1:0];
			ret=_table[i].value;
			if (!i) break;
		}
		if (ret==emptyValue) throw Decompressor::DecompressionError();
		return ret;
	}

	void insert(const HuffmanCode<T> &code)
	{
		if (code.value==emptyValue) throw Decompressor::DecompressionError();
		uint32_t i=0,length=uint32_t(_table.size());
		for (int32_t currentBit=code.length;currentBit>=0;currentBit--)
		{
			uint32_t codeBit=(currentBit)?(code.code&(1<<(currentBit-1)))?1:0:0;
			if (i!=length)
			{
				if (!currentBit || _table[i].value!=emptyValue) throw Decompressor::DecompressionError();
				uint32_t &tmp=_table[i].sub[codeBit];
				if (!tmp)
				{
					tmp=length;
					i=length;
				} else {
					i=tmp;
				}
			} else {
				_table.emplace_back((currentBit&&!codeBit)?length+1:0,(currentBit&&codeBit)?length+1:0,currentBit?emptyValue:code.value);
				length++;
				i++;
			}
		}
	}

private:
	std::vector<Node>	_table;
};

// create orderly Huffman table, as used by Deflate and Bzip2
template<typename T>
void CreateOrderlyHuffmanTable(T &dec,const uint8_t *bitLengths,uint32_t bitTableLength)
{
	uint8_t minDepth=32,maxDepth=0;
	// some optimization: more tables
	uint16_t firstIndex[33],lastIndex[33];
	uint16_t nextIndex[bitTableLength];
	for (uint32_t i=1;i<33;i++)
		firstIndex[i]=0xffffU;

	uint32_t realItems=0;
	for (uint32_t i=0;i<bitTableLength;i++)
	{
		uint8_t length=bitLengths[i];
		if (length>32) throw Decompressor::DecompressionError();
		if (length)
		{
			if (length<minDepth) minDepth=length;
			if (length>maxDepth) maxDepth=length;
			if (firstIndex[length]==0xffffU)
			{
				firstIndex[length]=i;
				lastIndex[length]=i;
			} else {
				nextIndex[lastIndex[length]]=i;
				lastIndex[length]=i;
			}
			realItems++;
		}
	}
	if (!maxDepth) throw Decompressor::DecompressionError();
	// optimization, the multiple depends how sparse the tree really is. (minimum is *2)
	// usually it is sparse.
	dec._table.reserve(realItems*3);

	uint32_t code=0;
	for (uint32_t depth=minDepth;depth<=maxDepth;depth++)
	{
		if (firstIndex[depth]!=0xffffU)
			nextIndex[lastIndex[depth]]=bitTableLength;

		for (uint32_t i=firstIndex[depth];i<bitTableLength;i=nextIndex[i])
		{
			dec.insert(typename T::CodeType{depth,code>>(maxDepth-depth),(typename T::ItemType)i});
			code+=1<<(maxDepth-depth);
		}
	}
}

#endif
