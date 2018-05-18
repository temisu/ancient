/* Copyright (C) Teemu Suutari */

#include "InputStream.hpp"
// for exceptions
#include "Decompressor.hpp"

ForwardInputStream::ForwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset) :
	_bufPtr(buffer.data()),
	_currentOffset(startOffset),
	_endOffset(endOffset)
{
	if (_currentOffset>_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size()) throw Decompressor::DecompressionError();
}

ForwardInputStream::~ForwardInputStream()
{
	// nothing needed
}

uint8_t ForwardInputStream::readByte()
{
	if (_currentOffset>=_endOffset) throw Decompressor::DecompressionError();
	uint8_t ret=_bufPtr[_currentOffset++];
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
	return ret;
}

const uint8_t *ForwardInputStream::consume(size_t bytes)
{
	if (_currentOffset+bytes>_endOffset) throw Decompressor::DecompressionError();
	const uint8_t *ret=&_bufPtr[_currentOffset];
	_currentOffset+=bytes;
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
	return ret;
}

BackwardInputStream::BackwardInputStream(const Buffer &buffer,size_t startOffset,size_t endOffset) :
	_bufPtr(buffer.data()),
	_currentOffset(endOffset),
	_endOffset(startOffset)
{
	if (_currentOffset<_endOffset || _currentOffset>buffer.size() || _endOffset>buffer.size()) throw Decompressor::DecompressionError();
}

BackwardInputStream::~BackwardInputStream()
{
	// nothing needed
}

uint8_t BackwardInputStream::readByte()
{
	if (_currentOffset<=_endOffset) throw Decompressor::DecompressionError();
	uint8_t ret=_bufPtr[--_currentOffset];
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
	return ret;
}

const uint8_t *BackwardInputStream::consume(size_t bytes)
{
	if (_currentOffset<_endOffset+bytes) throw Decompressor::DecompressionError();
	_currentOffset-=bytes;
	if (_linkedInputStream) _linkedInputStream->setOffset(_currentOffset);
	return &_bufPtr[_currentOffset];
}
