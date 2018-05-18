/* Copyright (C) Teemu Suutari */

#include <string.h>

#include "OutputStream.hpp"
// for exceptions
#include "Decompressor.hpp"

ForwardOutputStream::ForwardOutputStream(Buffer &buffer,size_t startOffset,size_t endOffset) :
	_bufPtr(buffer.data()),
	_startOffset(startOffset),
	_currentOffset(startOffset),
	_endOffset(endOffset)
{
	if (_startOffset>_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size()) throw Decompressor::DecompressionError();
}

ForwardOutputStream::~ForwardOutputStream()
{
	// nothing needed
}

void ForwardOutputStream::writeByte(uint8_t value)
{
	if (_currentOffset>=_endOffset) throw Decompressor::DecompressionError();
	_bufPtr[_currentOffset++]=value;
}

uint8_t ForwardOutputStream::copy(size_t distance,size_t count)
{
	if (!distance || _startOffset+distance>_currentOffset || _currentOffset+count>_endOffset) throw Decompressor::DecompressionError();
	uint8_t ret=0;
	for (size_t i=0;i<count;i++,_currentOffset++)
		ret=_bufPtr[_currentOffset]=_bufPtr[_currentOffset-distance];
	return ret;
}

uint8_t ForwardOutputStream::copy(size_t distance,size_t count,const Buffer &prevBuffer)
{
	size_t prevSize=prevBuffer.size();
	if (!distance || _startOffset+distance>_currentOffset+prevSize || _currentOffset+count>_endOffset) throw Decompressor::DecompressionError();
	size_t prevCount=0;
	uint8_t ret=0;
	if (_startOffset+distance>_currentOffset)
	{
		size_t prevDist=_startOffset+distance-_currentOffset;
		prevCount=std::min(count,prevDist);
		const uint8_t *prev=&prevBuffer[prevSize-prevDist];
		for (size_t i=0;i<prevCount;i++,_currentOffset++)
			ret=_bufPtr[_currentOffset]=prev[i];
	}
	for (size_t i=prevCount;i<count;i++,_currentOffset++)
		ret=_bufPtr[_currentOffset]=_bufPtr[_currentOffset-distance];
	return ret;
}

uint8_t ForwardOutputStream::copy(size_t distance,size_t count,uint8_t defaultChar)
{
	if (!distance || _currentOffset+count>_endOffset) throw Decompressor::DecompressionError();
	size_t prevCount=0;
	uint8_t ret=0;
	if (_startOffset+distance>_currentOffset)
	{
		prevCount=std::min(count,_startOffset+distance-_currentOffset);
		for (size_t i=0;i<prevCount;i++,_currentOffset++)
			ret=_bufPtr[_currentOffset]=defaultChar;
	}
	for (size_t i=prevCount;i<count;i++,_currentOffset++)
		ret=_bufPtr[_currentOffset]=_bufPtr[_currentOffset-distance];
	return ret;
}

void ForwardOutputStream::produce(const uint8_t *src,size_t bytes)
{
	if (_currentOffset+bytes>_endOffset) throw Decompressor::DecompressionError();
	::memcpy(&_bufPtr[_currentOffset],src,bytes);
	_currentOffset+=bytes;
}

BackwardOutputStream::BackwardOutputStream(Buffer &buffer,size_t startOffset,size_t endOffset) :
	_bufPtr(buffer.data()),
	_startOffset(startOffset),
	_currentOffset(endOffset),
	_endOffset(endOffset)
{
	if (_startOffset>_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size()) throw Decompressor::DecompressionError();
}

BackwardOutputStream::~BackwardOutputStream()
{
	// nothing needed
}

void BackwardOutputStream::writeByte(uint8_t value)
{
	if (_currentOffset<=_startOffset) throw Decompressor::DecompressionError();
	_bufPtr[--_currentOffset]=value;
}

uint8_t BackwardOutputStream::copy(size_t distance,size_t count)
{
	if (!distance || _startOffset+count>_currentOffset || _currentOffset+distance>_endOffset) throw Decompressor::DecompressionError();
	uint8_t ret=0;
	for (size_t i=0;i<count;i++,--_currentOffset)
		ret=_bufPtr[_currentOffset-1]=_bufPtr[_currentOffset+distance-1];
	return ret;
}