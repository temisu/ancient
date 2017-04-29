/* Copyright (C) Teemu Suutari */

#ifndef XPKMASTER_HPP
#define XPKMASTER_HPP

#include "Decompressor.hpp"
#include "XPKDecompressor.hpp"

class XPKMaster : public Decompressor
{
public:
	XPKMaster(const Buffer &packedData,bool allowRecursion=true);

	virtual ~XPKMaster();

	virtual bool isValid() const override final;
	virtual bool verifyPacked() const override final;
	virtual bool verifyRaw(const Buffer &rawData) const override final;

	virtual const std::string &getName() const override final;
	virtual size_t getPackedSize() const override final;
	virtual size_t getRawSize() const override final;

	virtual bool decompress(Buffer &rawData) override final;

	static bool detectHeader(uint32_t hdr);

private:
	std::unique_ptr<XPKDecompressor> createSubDecompressor(const Buffer &buffer,std::unique_ptr<XPKDecompressor::State> &state) const;

	template<class T>
	void registerDecompressor()
	{
		_decompressors.push_back(std::make_tuple(T::detectHeaderXPK,T::isRecursive,T::create));
	}

	template <typename F>
	bool forEachChunk(F func) const;

	const Buffer &_packedData;

	bool		_isValid=false;
	uint32_t	_packedSize=0;
	uint32_t	_rawSize=0;
	uint32_t	_headerSize=0;
	uint32_t	_type=0;
	bool		_longHeaders=false;

	std::vector<std::tuple<bool(*)(uint32_t),bool(*)(),std::unique_ptr<XPKDecompressor>(*)(uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&)>> _decompressors;
};

#endif
