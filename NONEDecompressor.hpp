/* Copyright (C) Teemu Suutari */

#ifndef NONEDECOMPRESSOR_HPP
#define NONEDECOMPRESSOR_HPP

#include "Decompressor.hpp"

// XPK sub-decompressor
class NONEDecompressor : public Decompressor
{
public:
	NONEDecompressor(uint32_t hdr,const Buffer &packedData);

	virtual ~NONEDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	bool		_isValid=false;
};

#endif
