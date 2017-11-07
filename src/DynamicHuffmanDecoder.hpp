/* Copyright (C) Teemu Suutari */

#ifndef DYNAMICHUFFMANDECODER_HPP
#define DYNAMICHUFFMANDECODER_HPP

#include <stddef.h>
#include <stdint.h>

// For exception
#include "Decompressor.hpp"

template<uint32_t codeCount>
class DynamicHuffmanDecoder
{
public:
	DynamicHuffmanDecoder()
	{
		reset();
	}

	~DynamicHuffmanDecoder()
	{
		// nothing needed
	}

	void reset()
	{
		for (uint32_t i=0;i<codeCount;i++)
		{
			_nodes[i].frequency=1;
			_nodes[i].code=-(i+1);

			_indexes[i]=codeCount-1-i;
		}
		for (uint32_t i=codeCount,j=0;i<codeCount*2-1;i++,j+=2)
		{
			_nodes[i].frequency=_nodes[j].frequency+_nodes[j+1].frequency;
			_nodes[i].code=j;

			_indexes[j+codeCount]=i;
			_indexes[j+codeCount+1]=i;
		}
	}

	template<typename F>
	uint32_t decode(F bitReader) const
	{
		int32_t code=_nodes[codeCount*2-2].code;
		while (code>=0)
			code=_nodes[code+bitReader()].code;
		return -(code+1);
	}

	void update(uint32_t code)
	{
		auto next=[&](int32_t index)->uint32_t
		{
			uint32_t ret=_indexes[index+codeCount];
			_nodes[ret].frequency++;
			return ret;
		};

		if (code>=codeCount) throw Decompressor::DecompressionError();
		for (uint32_t tmpIndex1=next(-code-1);tmpIndex1!=codeCount*2-2;tmpIndex1=next(tmpIndex1))
		{
			if (_nodes[tmpIndex1].frequency>_nodes[tmpIndex1+1].frequency)
			{
				uint32_t tmpIndex2=tmpIndex1;
				do {
					tmpIndex2++;
				} while (tmpIndex2<codeCount*2-2 && _nodes[tmpIndex1].frequency>_nodes[tmpIndex2+1].frequency);
				if (_nodes[tmpIndex1].code>=0) _indexes[_nodes[tmpIndex1].code+codeCount+1]=tmpIndex2;
				if (_nodes[tmpIndex2].code>=0) _indexes[_nodes[tmpIndex2].code+codeCount+1]=tmpIndex1;
				_indexes[_nodes[tmpIndex1].code+codeCount]=tmpIndex2;
				_indexes[_nodes[tmpIndex2].code+codeCount]=tmpIndex1;
				std::swap(_nodes[tmpIndex1].frequency,_nodes[tmpIndex2].frequency);
				std::swap(_nodes[tmpIndex1].code,_nodes[tmpIndex2].code);
				tmpIndex1=tmpIndex2;
			}
		}
	}

	// halve the frequencies rounding upwards
	void halve()
	{
		for (uint32_t i=0,j=0;i<codeCount*2-1;i++)
		{
			if (_nodes[i].code<0)
			{
				_nodes[j].frequency=(_nodes[i].frequency+1)>>1;
				_nodes[j].code=_nodes[i].code;
				j++;
			}
		}
		for (uint32_t i=codeCount,j=0;i<codeCount*2-1;i++,j+=2)
		{
			uint32_t freq=_nodes[j].frequency+_nodes[j+1].frequency;
			_nodes[i].frequency=freq;
			uint32_t k=i;
			while (freq<_nodes[k-1].frequency) k--;
			if (i!=k) ::memmove(&_nodes[k+1],&_nodes[k],sizeof(Node)*(i-k));
			_nodes[k].frequency=freq;
			_nodes[k].code=j;
		}
		for (uint32_t i=0;i<codeCount*2-1;i++)
		{
			int32_t code=_nodes[i].code;
			_indexes[code+codeCount]=i;
			if (code>=0) _indexes[code+codeCount+1]=i;
		}
	}

	uint32_t getMaxFrequency() const
	{
		return _nodes[codeCount*2-2].frequency;
	}

private:
	struct Node
	{
		uint32_t	frequency;
		int32_t		code;
	};

	Node			_nodes[codeCount*2-1];
	uint32_t		_indexes[codeCount*3-1];
};

#endif
