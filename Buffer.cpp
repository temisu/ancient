/* Copyright (C) Teemu Suutari */

#include "Buffer.hpp"

Buffer::Buffer()
{
	// nothing needed
}

Buffer::~Buffer()
{
	// nothing needed
}

bool Buffer::isResizable() const
{
	return false;
}

void Buffer::resize(size_t newSize)
{
	// nothing needed
}

template <>
uint8_t *GenericSubBuffer<Buffer>::data()
{
	return _base.data()+_start;
}

template <>
uint8_t *GenericSubBuffer<const Buffer>::data()
{
	// nope
	return nullptr;
}
