/* Copyright (C) Teemu Suutari */

#ifndef SHRIDECOMPRESSOR_HPP
#define SHRIDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"

class SHRIDecompressor : public XPKDecompressor
{
private:
	class SHRIState : public XPKDecompressor::State
	{
	public:
		SHRIState();
		virtual ~SHRIState();

		uint32_t vlen=0;
		uint32_t vnext=0;
		uint32_t shift=0;
		uint32_t ar[999];
	};

public:
	SHRIDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~SHRIDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

private:
	const Buffer				&_packedData;

	bool					_isValid=false;
	uint32_t				_ver=0;
	size_t					_startOffset=0;
	size_t					_rawSize=0;

	std::unique_ptr<XPKDecompressor::State>	&_state;	// reference!!!
};

#endif
