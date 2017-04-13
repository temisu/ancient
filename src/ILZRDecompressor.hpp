/* Copyright (C) Teemu Suutari */

#ifndef ILZRDECOMPRESSOR_HPP
#define ILZRDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class ILZRDecompressor : public XPKDecompressor
{
public:
	ILZRDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~ILZRDecompressor();

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
	size_t		_rawSize=0;
};

#endif
