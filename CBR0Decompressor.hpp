/* Copyright (C) Teemu Suutari */

#ifndef CBR0DECOMPRESSOR_HPP
#define CBR0DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class CBR0Decompressor : public XPKDecompressor
{
public:
	CBR0Decompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~CBR0Decompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
};

#endif
