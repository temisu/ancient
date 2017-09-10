/* Copyright (C) Teemu Suutari */

#ifndef CRMDECOMPRESSOR_HPP
#define CRMDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

class CRMDecompressor : public Decompressor, public XPKDecompressor
{
public:
	CRMDecompressor(const Buffer &packedData,uint32_t recursionLevel=0);
	CRMDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);
	virtual ~CRMDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getName() const override final;
	virtual const std::string &getSubName() const override final;

	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;
	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeader(uint32_t hdr);
	static bool detectHeaderXPK(uint32_t hdr);

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown);
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

private:
	const Buffer	&_packedData;

	bool		_isValid=false;
	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	bool		_isLZH=false;		// "normal" compression or LZH compression
	bool		_isSampled=false;	// normal or "sampled" i.e. obsfuscated
	bool		_isXPKDelta=false;	// If delta encoding defined in XPK

	static Decompressor::Registry<CRMDecompressor> _registration;
	static XPKDecompressor::Registry<CRMDecompressor> _XPKregistration;
};

#endif
