/* Copyright (C) Teemu Suutari */

#ifndef ARRAYBUFFER_HPP
#define ARRAYBUFFER_HPP

#include <stddef.h>
#include <stdint.h>

#include "Buffer.hpp"

template <size_t N>
class ArrayBuffer : public Buffer
{
public:
	ArrayBuffer(const ArrayBuffer&)=delete;
	ArrayBuffer& operator=(const ArrayBuffer&)=delete;

	ArrayBuffer() { }
	
	virtual ~ArrayBuffer() { }

	virtual const uint8_t *data() const override
	{
		return _data;
	}

	virtual uint8_t *data() override
	{
		return _data;
	}

	virtual size_t size() const override
	{
		return N;
	}

private:
	uint8_t _data[N];
};

#endif
