/* Copyright (C) Teemu Suutari */

#ifndef FREEZEDECOMPRESSOR_HPP
#define FREEZEDECOMPRESSOR_HPP

#include "Decompressor.hpp"

namespace ancient::internal
{

class FreezeDecompressor : public Decompressor
{
public:
	FreezeDecompressor(const Buffer &packedData,bool exactSizeKnown,bool verify);
	virtual ~FreezeDecompressor();

	virtual size_t getRawSize() const noexcept override final;
	virtual size_t getPackedSize() const noexcept override final;

	virtual const std::string &getName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,bool verify) override final;

	static bool detectHeader(uint32_t hdr) noexcept;

	static std::shared_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown,bool verify);

private:
	const Buffer	&_packedData;

	size_t		_packedSize=0;
	size_t		_rawSize=0;
	bool		_isOldVersion=false;
	bool		_exactSizeKnown=true;

	uint8_t		_hufTable[8];
};

}

#endif
