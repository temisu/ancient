/* Copyright (C) Teemu Suutari */

#ifndef DEFLATEDECOMPRESSOR_HPP
#define DEFLATEDECOMPRESSOR_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

class DEFLATEDecompressor : public Decompressor, public XPKDecompressor
{
public:
	DEFLATEDecompressor(const Buffer &packedData,bool exactSizeKnown);
	DEFLATEDecompressor(uint32_t hdr,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state);
	DEFLATEDecompressor(const Buffer &packedData,size_t packedSize,size_t rawSize);	// completely raw stream
	virtual ~DEFLATEDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual size_t getPackedSize() const override final;

	virtual const std::string &getName() const override final;
	virtual const std::string &getSubName() const override final;

	virtual bool decompress(Buffer &rawData) override final;
	virtual bool decompress(Buffer &rawData,const Buffer &previousData) override final;

	static bool detectHeader(uint32_t hdr);
	static bool detectHeaderXPK(uint32_t hdr);

private:
	bool detectGZIP();
	bool detectZLib();

	enum class Type
	{
		GZIP=0,
		ZLib,
		Raw
	};

	const Buffer &_packedData;

	bool		_isValid=false;
	size_t		_packedSize=0;
	size_t		_packedOffset=0;
	size_t		_rawSize=0;
	uint32_t	_rawCRC;
	Type		_type;
	bool		_exactSizeKnown=true;
};

#endif
