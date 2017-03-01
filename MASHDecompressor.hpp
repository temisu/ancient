/* Copyright (C) Teemu Suutari */

#ifndef MASHDECOMPRESSOR_HPP
#define MASHDECOMPRESSOR_HPP

#include "Decompressor.hpp"

// XPK sub-decompressor
class MASHDecompressor : public Decompressor
{
public:
	MASHDecompressor(uint32_t hdr,const Buffer &packedData);

	virtual ~MASHDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	bool		_isValid;
};

#endif
