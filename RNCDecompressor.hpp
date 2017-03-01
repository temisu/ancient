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

	virtual size_t getRawSize() const override final;
	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	enum class RNCVersion
	{
		Invalid=0,
		RNC1Old,
		RNC1New,
		RNC2
	};

	bool RNC1DecompressOld(Buffer &rawData);
	bool RNC1DecompressNew(Buffer &rawData);
	bool RNC2Decompress(Buffer &rawData);

	bool		_isValid;
	uint32_t	_rawSize;
	uint32_t	_packedSize;
	uint16_t	_rawCRC;
	uint16_t	_packedCRC;
	uint8_t		_chunks;
	RNCVersion	_ver;
};

#endif
