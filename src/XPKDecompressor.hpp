/* Copyright (C) Teemu Suutari */

#ifndef XPKDECOMPRESSOR_HPP
#define XPKDECOMPRESSOR_HPP

#include <stddef.h>
#include <stdint.h>

#include <string>

#include <Buffer.hpp>
#include "Decompressor.hpp"


class XPKDecompressor
{
public:
	class State
	{
	public:
		State(const State&)=delete;
		State& operator=(const State&)=delete;

		State()=default;
		virtual ~State();

		uint32_t getRecursionLevel() const;
	};

	XPKDecompressor(const XPKDecompressor&)=delete;
	XPKDecompressor& operator=(const XPKDecompressor&)=delete;

	XPKDecompressor(uint32_t recursionLevel=0);
	virtual ~XPKDecompressor();

	// if the data-stream is constructed properly, return true
	virtual bool isValid() const=0;
	// check the packedData for errors: CRC or other checksum if available
	virtual bool verifyPacked() const=0;
	virtual bool verifyRaw(const Buffer &rawData) const=0;

	virtual const std::string &getSubName() const;

	// Actual decompression
	virtual bool decompress(Buffer &rawData,const Buffer &previousData)=0;

	template<class T>
	class Registry
	{
	public:
		Registry()
		{
			XPKDecompressor::registerDecompressor(T::detectHeaderXPK,T::create);
		}

		~Registry()
		{
			// TODO: no cleanup yet
		}
	};

private:
	static void registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<XPKDecompressor>(*create)(uint32_t,uint32_t,const Buffer&,std::unique_ptr<XPKDecompressor::State>&));

protected:
	uint32_t	_recursionLevel;
};

#endif
