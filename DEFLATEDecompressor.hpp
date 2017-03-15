/* Copyright (C) Teemu Suutari */

#ifndef DEFLATEDECOMPRESSOR_HPP
#define DEFLATEDECOMPRESSOR_HPP

#include "Decompressor.hpp"

class DEFLATEDecompressor : public Decompressor
{
public:
	DEFLATEDecompressor(const Buffer &packedData);
	DEFLATEDecompressor(uint32_t hdr,const Buffer &packedData);				// XPK sub-decompressor
	DEFLATEDecompressor(const Buffer &packedData,uint32_t packedSize,uint32_t rawSize);	// completely raw stream
	virtual ~DEFLATEDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual size_t getRawSize() const override final;
	virtual const std::string &getName() const override final;
	virtual size_t getPackedSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);
	static bool detectHeaderXPK(uint32_t hdr);

protected:
	virtual const std::string &getSubName() const override final;

private:
	bool detectGZIP();
	bool detectZLib();

	enum class Type
	{
		GZIP=0,
		ZLib,
		Raw
	};

	bool		_isValid=false;
	uint32_t	_packedSize=0;
	uint32_t	_packedOffset=0;
	uint32_t	_rawSize=0;
	uint32_t	_rawCRC;
	Type		_type;
};

#endif
