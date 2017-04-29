/* Copyright (C) Teemu Suutari */

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <stddef.h>
#include <stdint.h>

class Buffer
{
protected:
	Buffer();

public:
	Buffer(const Buffer&)=delete;
	Buffer& operator=(const Buffer&)=delete;

	virtual ~Buffer();

	virtual const uint8_t *data() const=0;
	virtual uint8_t *data()=0;
	virtual size_t size() const=0;

	virtual bool isResizable() const;
	virtual void resize(size_t newSize);

	uint8_t &operator[](size_t i);
	const uint8_t &operator[](size_t i) const;

	bool readBE(size_t offset,uint32_t &retValue) const;
	bool readBE(size_t offset,uint16_t &retValue) const;

	bool readLE(size_t offset,uint64_t &retValue) const;
	bool readLE(size_t offset,uint32_t &retValue) const;
	bool readLE(size_t offset,uint16_t &retValue) const;

	bool read(size_t offset,uint8_t &retValue) const;
};

#endif
