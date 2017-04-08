/* Copyright (C) Teemu Suutari */

#ifndef IMPDECOMPRESSOR_HPP
#define IMPDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

class IMPDecompressor : public Decompressor, public XPKDecompressor
{
public:
	IMPDecompressor(const Buffer &packedData);
	IMPDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);
	virtual ~IMPDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getName() const override final;
	virtual const std::string &getSubName() const override final;

	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;
	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeader(uint32_t hdr);
	static bool detectHeaderXPK(uint32_t hdr);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
	uint32_t	_rawSize=0;
	uint32_t	_endOffset=0;
	uint32_t	_checksumAddition=0;
	uint32_t	_checksum=0;
	bool		_isXPK=false;
};

#endif
