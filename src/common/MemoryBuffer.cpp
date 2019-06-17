/* Copyright (C) Teemu Suutari */

#include <string.h>
#include <stdlib.h>

#ifdef __linux__
#include <bsd/stdlib.h>
#endif

#include <memory>

#include "MemoryBuffer.hpp"

MemoryBuffer::MemoryBuffer(size_t size) :
	_data(reinterpret_cast<uint8_t*>(::malloc(size))),
	_size(size)
{
	if (!_data) throw OutOfMemoryError();
}

MemoryBuffer::MemoryBuffer(const Buffer &src,size_t offset,size_t size) :
	MemoryBuffer(size)
{
	if(offset+size>src.size()) throw InvalidOperationError();
	::memcpy(_data,src.data()+offset,size);
}


MemoryBuffer::~MemoryBuffer()
{
	::free(_data);
}

const uint8_t *MemoryBuffer::data() const noexcept
{
	return _data;
}

uint8_t *MemoryBuffer::data()
{
	return _data;
}

size_t MemoryBuffer::size() const noexcept
{
	return _size;
}

bool MemoryBuffer::isResizable() const noexcept
{
	return true;
}

void MemoryBuffer::resize(size_t newSize) 
{
	_data=reinterpret_cast<uint8_t*>(::reallocf(_data,newSize));
	if (!_data) throw OutOfMemoryError();
}
