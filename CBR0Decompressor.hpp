/* Copyright (C) Teemu Suutari */

#ifndef CBR0DECOMPRESSOR_HPP
#define CBR0DECOMPRESSOR_HPP

#include "Decompressor.hpp"

// XPK sub-decompressor
class CBR0Decompressor : public Decompressor
{
public:
	CBR0Decompressor(uint32_t hdr,const Buffer &packedData);

	virtual ~CBR0Decompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeaderXPK(uint32_t hdr);

protected:
	virtual const std::string &getSubName() const override final;

private:
	bool		_isValid=false;
};

#endif
