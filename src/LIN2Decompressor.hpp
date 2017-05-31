/* Copyright (C) Teemu Suutari */

#ifndef LIN2DECOMPRESSOR_HPP
#define LIN2DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class LIN2Decompressor : public XPKDecompressor
{
public:
	LIN2Decompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~LIN2Decompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);
	static bool isRecursive();
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
	uint32_t	_ver=0;
	size_t		_endStreamOffset=0;
	size_t		_midStreamOffset=0;
};

#endif
