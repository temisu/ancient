/* Copyright (C) Teemu Suutari */

#ifndef XPKDECOMPRESSOR_HPP
#define XPKDECOMPRESSOR_HPP

#include <stddef.h>
#include <stdint.h>

#include <string>

#include <Buffer.hpp>
#include "Decompressor.hpp"


class XPKDecompressor
{
public:
	class State
	{
	public:
		State()=default;

		State(const State&)=delete;
		State& operator=(const State&)=delete;

		virtual ~State();
	};

	XPKDecompressor()=default;

	XPKDecompressor(const XPKDecompressor&)=delete;
	XPKDecompressor& operator=(const XPKDecompressor&)=delete;

	virtual ~XPKDecompressor();

	// if the data-stream is constructed properly, return true
	virtual bool isValid() const=0;
	// check the packedData for errors: CRC or other checksum if available
	virtual bool verifyPacked() const=0;
	virtual bool verifyRaw(const Buffer &rawData) const=0;

	virtual const std::string &getSubName() const;

	// Actual decompression
	virtual bool decompress(Buffer &rawData,const Buffer &previousData)=0;
};

#endif
