/* Copyright (C) Teemu Suutari */

#ifndef NONEDECOMPRESSOR_HPP
#define NONEDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class NONEDecompressor : public XPKDecompressor
{
public:
	NONEDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~NONEDecompressor();

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
