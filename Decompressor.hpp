/* Copyright (C) Teemu Suutari */

#ifndef DECOMPRESSOR_HPP
#define DECOMPRESSOR_HPP

#include <stddef.h>
#include <stdint.h>

#include <string>

#include <Buffer.hpp>

constexpr uint32_t FourCC(uint32_t cc) noexcept
{
	// return with something else if behavior is not same as in clang/gcc for multibyte literals
	return cc;
}

class XPKMaster;

class Decompressor
{
friend class XPKMaster;
protected:
	Decompressor(const Buffer &packedData);

public:
	Decompressor(const Decompressor&)=delete;
	Decompressor& operator=(const Decompressor&)=delete;

	virtual ~Decompressor();

	// if the data-stream is constructed properly, return true
	virtual bool isValid() const=0;
	// check the packedData for errors: CRC or other checksum if available
	virtual bool verifyPacked() const=0;
	virtual bool verifyRaw(const Buffer &rawData) const=0;

	// Bookkeeping
	// Name is human readable long name
	virtual const std::string &getName() const;
	// PackedSize or RawSize are taken from the stream
	// if available. 0 otherwise. Sometimes decompression will update the size
	// f.e. TPWM
	virtual size_t getPackedSize() const=0;
	virtual size_t getRawSize() const=0;

	// Actual decompression
	virtual bool decompress(Buffer &rawData)=0;

protected:
	virtual const std::string &getSubName() const;	// for XPK

	const Buffer &_packedData;
};

Decompressor *CreateDecompressor(const Buffer &packedData);

#endif
