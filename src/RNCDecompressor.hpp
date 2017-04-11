/* Copyright (C) Teemu Suutari */

#ifndef RNCDECOMPRESSOR_HPP
#define RNCDECOMPRESSOR_HPP

#include "Decompressor.hpp"

class RNCDecompressor : public Decompressor
{
public:
	RNCDecompressor(const Buffer &packedData);

	virtual ~RNCDecompressor();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getName() const override final;
	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	enum class Version
	{
		RNC1Old=0,
		RNC1New,
		RNC2
	};

	bool RNC1DecompressOld(Buffer &rawData);
	bool RNC1DecompressNew(Buffer &rawData);
	bool RNC2Decompress(Buffer &rawData);

	const Buffer &_packedData;

	bool		_isValid=false;
	uint32_t	_rawSize=0;
	uint32_t	_packedSize=0;
	uint16_t	_rawCRC=0;
	uint16_t	_packedCRC=0;
	uint8_t		_chunks=0;
	Version		_ver;
};

#endif
