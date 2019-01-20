/* Copyright (C) Teemu Suutari */

#ifndef SXSCDECOMPRESSOR_HPP
#define SXSCDECOMPRESSOR_HPP

#include <stdint.h>

#include "XPKDecompressor.hpp"
#include "InputStream.hpp"

class SXSCDecompressor : public XPKDecompressor
{
public:
	SXSCDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~SXSCDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	void decompressASC(Buffer &rawData,ForwardInputStream &inputStream);
	void decompressHSC(Buffer &rawData,ForwardInputStream &inputStream);

	class ArithDecoder {
	public:
		ArithDecoder(ForwardInputStream &inputStream);
		~ArithDecoder();

		uint16_t decode(uint16_t length);
		void scale(uint16_t newLow,uint16_t newHigh,uint16_t newRange);

	private:
		MSBBitReader<ForwardInputStream>	_bitReader;

		uint16_t				_low=0;
		uint16_t				_high=0xffffU;
		uint16_t				_stream;
	};

	const Buffer					&_packedData;
	bool						_isHSC;

	static XPKDecompressor::Registry<SXSCDecompressor> _XPKregistration;
};

#endif
