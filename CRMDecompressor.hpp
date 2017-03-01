/* Copyright (C) Teemu Suutari */

#ifndef CRMDECOMPRESSOR_HPP
#define CRMDECOMPRESSOR_HPP

#include "Decompressor.hpp"

class CRMDecompressor : public Decompressor
{
public:
	CRMDecompressor(const Buffer &packedData);
	virtual ~CRMDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	bool		_isValid;
	uint32_t	_packedSize;
	uint32_t	_rawSize;
	bool		_isLZH;			// "normal" compression or LZH compression
	bool		_isSampled;		// normal or "sampled" i.e. obsfuscated
};

#endif
