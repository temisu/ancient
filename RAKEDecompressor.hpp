/* Copyright (C) Teemu Suutari */

#ifndef RAKEDECOMPRESSOR_HPP
#define RAKEDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class RAKEDecompressor : public XPKDecompressor
{
public:
	RAKEDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~RAKEDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeaderXPK(uint32_t hdr);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
	size_t		_midStreamOffset=0;
};

#endif
