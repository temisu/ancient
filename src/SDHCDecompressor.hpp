/* Copyright (C) Teemu Suutari */

#ifndef SDHCDECOMPRESSOR_HPP
#define SDHCDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class SDHCDecompressor : public XPKDecompressor
{
public:
	SDHCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~SDHCDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

private:
	const Buffer	&_packedData;

	bool		_isValid=false;
	uint16_t	_mode=0;

	static XPKDecompressor::Registry<SDHCDecompressor> _XPKregistration;
};

#endif
