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

uint8_t &Buffer::operator[](size_t i)
{
	static uint8_t dummy=0;
	if (i>=size()) return dummy;
	return data()[i];
}

const uint8_t &Buffer::operator[](size_t i) const
{
	static uint8_t dummy=0;
	if (i>=size()) return dummy;
	return data()[i];
}

bool Buffer::readBE(size_t offset,uint32_t &retValue) const
{
	if (offset+4>size()) return false;
	const uint8_t *ptr=data()+offset;
	retValue=(uint32_t(ptr[0])<<24)|(uint32_t(ptr[1])<<16)|(uint32_t(ptr[2])<<8)|uint32_t(ptr[3]);
	return true;
}

bool Buffer::readBE(size_t offset,uint16_t &retValue) const
{
	if (offset+2>size()) return false;
	const uint8_t *ptr=data()+offset;
	retValue=(uint16_t(ptr[0])<<8)|uint16_t(ptr[1]);
	return true;
}

bool Buffer::readLE(size_t offset,uint64_t &retValue) const
{
	if (offset+8>size()) return false;
	const uint8_t *ptr=data()+offset;
	retValue=(uint64_t(ptr[7])<<56)|(uint64_t(ptr[6])<<48)|(uint64_t(ptr[5])<<40)|(uint64_t(ptr[4])<<32)|
		(uint64_t(ptr[3])<<24)|(uint64_t(ptr[2])<<16)|(uint64_t(ptr[1])<<8)|uint64_t(ptr[0]);
	return true;
}

bool Buffer::readLE(size_t offset,uint32_t &retValue) const
{
	if (offset+4>size()) return false;
	const uint8_t *ptr=data()+offset;
	retValue=(uint32_t(ptr[3])<<24)|(uint32_t(ptr[2])<<16)|(uint32_t(ptr[1])<<8)|uint32_t(ptr[0]);
	return true;
}

bool Buffer::readLE(size_t offset,uint16_t &retValue) const
{
	if (offset+2>size()) return false;
	const uint8_t *ptr=data()+offset;
	retValue=(uint16_t(ptr[1])<<8)|uint16_t(ptr[0]);
	return true;
}

bool Buffer::read(size_t offset,uint8_t &retValue) const
{
	if (offset>=size()) return false;
	const uint8_t *ptr=reinterpret_cast<const uint8_t*>(data())+offset;
	retValue=ptr[0];
	return true;
}
