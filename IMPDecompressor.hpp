/* Copyright (C) Teemu Suutari */

#ifndef IMPDECOMPRESSOR_HPP
#define IMPDECOMPRESSOR_HPP

#include "Decompressor.hpp"

class IMPDecompressor : public Decompressor
{
public:
	IMPDecompressor(const Buffer &packedData);
	virtual ~IMPDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	bool		_isValid;
	uint32_t	_rawSize;
	uint32_t	_endOffset;
	uint32_t	_checksumAddition;
	uint32_t	_checksum;
};

#endif
