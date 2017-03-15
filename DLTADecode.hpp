/* Copyright (C) Teemu Suutari */

#ifndef DLTADECODE_HPP
#define DLTADECODE_HPP

#include "Decompressor.hpp"

// XPK sub-decompressor
class DLTADecode : public Decompressor
{
public:
	DLTADecode(uint32_t hdr,const Buffer &packedData);

	virtual ~DLTADecode();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeaderXPK(uint32_t hdr);

	// static method for easy external usage. Buffers can be the same for in-place replacement
	static bool decode(Buffer &bufferDest,const Buffer &bufferSrc,size_t offset,size_t size);
protected:
	virtual const std::string &getSubName() const override final;

private:
	bool		_isValid=false;
};

#endif
