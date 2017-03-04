/* Copyright (C) Teemu Suutari */

#ifndef SQSHDECOMPRESSOR_HPP
#define SQSHDECOMPRESSOR_HPP

#include "Decompressor.hpp"

// XPK sub-decompressor
class SQSHDecompressor : public Decompressor
{
public:
	SQSHDecompressor(uint32_t hdr,const Buffer &packedData);

	virtual ~SQSHDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	bool		_isValid=false;
	uint32_t	_rawSize=0;
};

#endif
