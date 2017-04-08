/* Copyright (C) Teemu Suutari */

#ifndef SHR3DECOMPRESSOR_HPP
#define SHR3DECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class SHR3Decompressor : public XPKDecompressor
{
private:
	class SHR3State : public XPKDecompressor::State
	{
	public:
		SHR3State();
		virtual ~SHR3State();

		uint32_t vlen=0;
		uint32_t vnext=0;
		uint32_t shift=0;
		uint32_t ar[999];
	};

public:
	SHR3Decompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~SHR3Decompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);

private:
	const Buffer &_packedData;

	bool					_isValid=false;
	uint32_t				_ver=0;

	std::unique_ptr<XPKDecompressor::State>	&_state;	// reference!!!
};

#endif
