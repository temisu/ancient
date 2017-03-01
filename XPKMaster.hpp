/* Copyright (C) Teemu Suutari */

#ifndef XPKMASTER_HPP
#define XPKMASTER_HPP

#include "Decompressor.hpp"

class XPKMaster : public Decompressor
{
public:
	XPKMaster(const Buffer &packedData);

	virtual ~XPKMaster();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	Decompressor *createSubDecompressor(const Buffer &buffer) const;

	template <typename F>
	bool forEachChunk(F func) const;

	bool		_isValid;
	uint32_t	_packedSize;
	uint32_t	_rawSize;
	uint32_t	_headerSize;
	uint32_t	_type;
	bool		_longHeaders;
};

#endif
