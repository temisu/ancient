/* Copyright (C) Teemu Suutari */

#ifndef XPKMASTER_HPP
#define XPKMASTER_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

class XPKMaster : public Decompressor
{
friend class XPKDecompressor;
public:
	XPKMaster(const Buffer &packedData,uint32_t recursionLevel=0);

	virtual ~XPKMaster();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getName() const override final;
	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

	static std::unique_ptr<Decompressor> create(const Buffer &packedData,bool exactSizeKnown);

	// Can be used to create directly decoder for chunk (needed by CYB2)
	static std::unique_ptr<XPKDecompressor> createDecompressor(uint32_t type,uint32_t recursionLevel,const Buffer &buffer,std::unique_ptr<XPKDecompressor::State> &state);

private:
	static void registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<XPKDecompressor>(*create)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&));
	static constexpr uint32_t getMaxRecursionLevel() noexcept { return 4; }

	template <typename F>
	bool forEachChunk(F func) const;

	const Buffer	&_packedData;

	bool		_isValid=false;
	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	uint32_t	_headerSize=0;
	uint32_t	_type=0;
	bool		_longHeaders=false;
	uint32_t	_recursionLevel=0;

	static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<XPKDecompressor>(*)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&)>> *_XPKDecompressors;

	static Decompressor::Registry<XPKMaster> _registration;
};

#endif
