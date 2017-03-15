/* Copyright (C) Teemu Suutari */

#ifndef TPWMDECOMPRESSOR_HPP
#define TPWMDECOMPRESSOR_HPP

#include "Decompressor.hpp"

class TPWMDecompressor : public Decompressor
{
public:
	TPWMDecompressor(const Buffer &packedData);

	virtual ~TPWMDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getName() const override final;
	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	bool		_isValid=false;
	uint32_t	_rawSize=0;
	size_t		_decompressedPackedSize=0;
};

#endif
