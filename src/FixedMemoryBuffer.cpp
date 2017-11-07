/* Copyright (C) Teemu Suutari */

#include <string.h>

#include <memory>

#include <FixedMemoryBuffer.hpp>

FixedMemoryBuffer::FixedMemoryBuffer(size_t size) :
	_data(std::make_unique<uint8_t[]>(size)),
	_size(size)
{
	// nothing needed
}

FixedMemoryBuffer::FixedMemoryBuffer(const Buffer &src,size_t offset,size_t size) :
	FixedMemoryBuffer(size)
{
	if(offset+size>src.size()) throw InvalidOperationError();
	::memcpy(_data.get(),src.data()+offset,size);
}


FixedMemoryBuffer::~FixedMemoryBuffer()
{
	// nothing needed
}

const uint8_t *FixedMemoryBuffer::data() const noexcept
{
	return _data.get();
}

uint8_t *FixedMemoryBuffer::data()
{
	return _data.get();
}

size_t FixedMemoryBuffer::size() const noexcept
{
	return _size;
}

bool FixedMemoryBuffer::isResizable() const noexcept
{
	return false;
}

void FixedMemoryBuffer::resize(size_t newSize) 
{
	throw InvalidOperationError();
}
