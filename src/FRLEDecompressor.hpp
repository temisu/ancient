/* Copyright (C) Teemu Suutari */

#ifndef FRLEDECOMPRESSOR_HPP
#define FRLEDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class FRLEDecompressor : public XPKDecompressor
{
public:
	FRLEDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~FRLEDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
};

#endif
