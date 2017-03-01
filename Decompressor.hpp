/* Copyright (C) Teemu Suutari */

#ifndef DECOMPRESSOR_HPP
#define DECOMPRESSOR_HPP

#include <stddef.h>
#include <stdint.h>

#include "Buffer.hpp"

constexpr uint32_t FourCC(uint32_t cc) noexcept
{
	// return with something else if behavior is not same as in clang/gcc for multibyte literals
	return cc;
}

class Decompressor
{
protected:
	Decompressor(const Buffer &packedData);

public:
	Decompressor(const Decompressor&)=delete;
	Decompressor& operator=(const Decompressor&)=delete;

	virtual ~Decompressor();

	// if the data-stream is constructed properly, return true
	virtual bool isValid() const=0;
	// check the packedData for errors (f.e. CRC)
	virtual bool verifyPacked() const=0;
	virtual bool verifyRaw(const Buffer &rawData) const=0;

	virtual size_t getRawSize() const=0;
	virtual bool decompress(Buffer &rawData)=0;

protected:
	const Buffer &_packedData;
};

Decompressor *CreateDecompressor(const Buffer &packedData);

#endif
