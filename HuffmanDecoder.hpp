/* Copyright (C) Teemu Suutari */

#ifndef HUFFMANDECODER_HPP
#define HUFFMANDECODER_HPP

#include <stddef.h>
#include <stdint.h>

#include <initializer_list>
#include <vector>

template<typename T>
struct HuffmanCode
{
	size_t	length;
	size_t	code;

	T	value;
};

template<typename T,T emptyValue,size_t depth>
class HuffmanDecoder
{
private:
	static const size_t _length=(2<<depth)-2;

public:
	HuffmanDecoder() :
		_isValid(true)
	{
		for (size_t i=0;i<_length;i++) _table[i]=emptyValue;
	}

	HuffmanDecoder(std::initializer_list<HuffmanCode<T>> list) :
		HuffmanDecoder()
	{
		for (auto &item : list)
			insert(item);
	}


	~HuffmanDecoder()
	{
	}


	bool isValid() { return _isValid; }
	
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
		return ret;
	}

	void insert(const HuffmanCode<T> &code)
	{
		if (code.length>depth)
		{
			_isValid=false;
			return;
		}
		for (size_t i=0,j=code.length;j!=0;j--)
		{
			if (code.code&(1<<(j-1))) i++;
			if (_table[i]!=emptyValue)
			{
				_isValid=false;
				return;
			}
			if (j==1) _table[i]=code.value;
			i=i*2+2;
		}
	}

private:
	bool	_isValid;
	T	_table[_length];
};

// better for non-fixed sizes (especially deep ones)

template<typename T,T emptyValue>
class HuffmanDecoder<T,emptyValue,0>
{
private:
	struct Node
	{
		size_t	sub[2];
		T	value;
	};

public:
	HuffmanDecoder() :
		_isValid(true)
	{
		_table.push_back(Node{{0,0},emptyValue});
	}

	HuffmanDecoder(std::initializer_list<HuffmanCode<T>> list) :
		HuffmanDecoder()
	{
		for (auto &item : list)
			insert(item);
	}


	~HuffmanDecoder()
	{
	}

	bool isValid() { return _isValid; }

	template<typename F>
	T decode(F bitReader) const
	{
		size_t length=_table.size();
		T ret=emptyValue;
		size_t i=0;
		while (i<length && ret==emptyValue)
		{
			i=_table[i].sub[bitReader()?1:0];
			ret=_table[i].value;
			if (!i) break;
		}
		return ret;
	}

	void insert(const HuffmanCode<T> &code)
	{
		size_t length=_table.size();
		for (ssize_t i=0,currentBit=code.length;currentBit>=0;currentBit--)
		{
			size_t codeBit=(currentBit)?(code.code&(1<<(currentBit-1)))?1:0:0;
			if (i==ssize_t(length))
			{
				_table.push_back(Node{{(currentBit&&!codeBit)?length+1:0,(currentBit&&codeBit)?length+1:0},currentBit?emptyValue:code.value});
				length++;
				i++;
			} else {
				if (!currentBit || _table[i].value!=emptyValue)
				{
					_isValid=false;
					return;
				}
				size_t tmp=_table[i].sub[codeBit];
				if (!tmp)
				{
					_table[i].sub[codeBit]=length;
					i=length;
				} else {
					i=tmp;
				}
			}
		}
	}

private:
	bool			_isValid;
	std::vector<Node>	_table;
};

#endif
