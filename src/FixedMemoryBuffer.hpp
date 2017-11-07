/* Copyright (C) Teemu Suutari */

#ifndef FIXEDMEMORYBUFFER_HPP
#define FIXEDMEMORYBUFFER_HPP

#include <memory>

#include "Buffer.hpp"

class FixedMemoryBuffer : public Buffer
{
public:
	FixedMemoryBuffer(size_t size);
	FixedMemoryBuffer(const Buffer &src,size_t offset,size_t size);
	virtual ~FixedMemoryBuffer() override final;

	virtual const uint8_t *data() const noexcept override final;
	virtual uint8_t *data() override final;
	virtual size_t size() const noexcept override final;

	virtual bool isResizable() const noexcept override final;
	virtual void resize(size_t newSize) override final;

private:
	std::unique_ptr<uint8_t[]>	_data;
	size_t				_size;
};

#endif
