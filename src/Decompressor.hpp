/* Copyright (C) Teemu Suutari */

#ifndef DECOMPRESSOR_HPP
#define DECOMPRESSOR_HPP

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <memory>

#include <Common.hpp>
#include <Buffer.hpp>

class Decompressor
{
public:
	Decompressor()=default;

	Decompressor(const Decompressor&)=delete;
	Decompressor& operator=(const Decompressor&)=delete;

	virtual ~Decompressor();

	// if the data-stream is constructed properly, return true
	virtual bool isValid() const=0;
	// check the packedData for errors: CRC or other checksum if available
	virtual bool verifyPacked() const=0;
	virtual bool verifyRaw(const Buffer &rawData) const=0;

	// Book keeping
	// Name is human readable long name
	virtual const std::string &getName() const;
	// PackedSize or RawSize are taken from the stream
	// if available. 0 otherwise. Sometimes decompression will update the size
	// f.e. TPWM
	virtual size_t getPackedSize() const=0;
	virtual size_t getRawSize() const=0;

	// Actual decompression
	virtual bool decompress(Buffer &rawData)=0;

	// the functions are there to protect against "accidental" large files when parsing headers
	// a.k.a. 16M should be enough for everybody (sizes do not have to accurate i.e.
	// compressors can exclude header content for simplification)
	static constexpr size_t getMaxPackedSize() noexcept { return 0x100'0000U; }
	static constexpr size_t getMaxRawSize() noexcept { return 0x100'0000U; }
	// This is for limiting memory usage of the algorithms. Again not a hard limit but rule of the thumb
	static constexpr size_t getMaxMemorySize() noexcept { return 0x10'0000U; }
};

std::unique_ptr<Decompressor> CreateDecompressor(const Buffer &packedData,bool exactSizeKnown);

#endif
