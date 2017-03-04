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
	bool		_isValid=false;
	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	bool		_isLZH=false;		// "normal" compression or LZH compression
	bool		_isSampled=false;	// normal or "sampled" i.e. obsfuscated
};

#endif
