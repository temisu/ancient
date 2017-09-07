/* Copyright (C) Teemu Suutari */

#ifndef DLTADECODE_HPP
#define DLTADECODE_HPP

#include "XPKDecompressor.hpp"

class DLTADecode : public XPKDecompressor
{
public:
	DLTADecode(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	virtual ~DLTADecode();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeaderXPK(uint32_t hdr);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

	// static method for easy external usage. Buffers can be the same for in-place replacement
	static bool decode(Buffer &bufferDest,const Buffer &bufferSrc,size_t offset,size_t size);

private:
	const Buffer	&_packedData;

	bool		_isValid=false;
};

#endif
