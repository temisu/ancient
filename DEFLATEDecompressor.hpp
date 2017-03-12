/* Copyright (C) Teemu Suutari */

#ifndef DEFLATEDECOMPRESSOR_HPP
#define DEFLATEDECOMPRESSOR_HPP

#include "Decompressor.hpp"

class DEFLATEDecompressor : public Decompressor
{
public:
	DEFLATEDecompressor(const Buffer &packedData);
	DEFLATEDecompressor(uint32_t hdr,const Buffer &packedData);	// XPK sub-decompressor
	virtual ~DEFLATEDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);
	static bool detectHeaderXPK(uint32_t hdr);

private:
	bool		_isZlibStream=false;
	bool		_isValid=false;
	uint32_t	_packedSize=0;
	uint32_t	_packedOffset=0;
	uint32_t	_rawSize=0;
	uint32_t	_rawCRC;
};

#endif
