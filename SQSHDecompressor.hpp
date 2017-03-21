/* Copyright (C) Teemu Suutari */

#ifndef SQSHDECOMPRESSOR_HPP
#define SQSHDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class SQSHDecompressor : public XPKDecompressor
{
public:
	SQSHDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~SQSHDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeaderXPK(uint32_t hdr);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
	uint32_t	_rawSize=0;
};

#endif
