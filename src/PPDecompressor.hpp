/* Copyright (C) Teemu Suutari */

#ifndef PPDECOMPRESSOR_HPP
#define PPDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"


class PPDecompressor : public Decompressor, public XPKDecompressor
{
private:
	class PPState : public XPKDecompressor::State
	{
	public:
		PPState(uint32_t mode);
		virtual ~PPState();

		uint32_t _cachedMode;
	};

public:
	PPDecompressor(const Buffer &packedData,bool exactSizeKnown);
	PPDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);
	virtual ~PPDecompressor();

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
	static bool isRecursive();
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);

private:
	const Buffer &_packedData;

	bool		_isValid=false;
	size_t		_dataStart=0;
	size_t		_rawSize=0;
	uint8_t		_startShift=0;
	uint8_t		_modeTable[4];
	bool		_isXPK=false;
};


#endif
