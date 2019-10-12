/* Copyright (C) Teemu Suutari */

#ifndef ARTMDECOMPRESSOR_HPP
#define ARTMDECOMPRESSOR_HPP

#include "XPKDecompressor.hpp"
#include "InputStream.hpp"

class ARTMDecompressor : public XPKDecompressor
{
public:
	ARTMDecompressor(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

	virtual ~ARTMDecompressor();

	virtual const std::string &getSubName() const noexcept override final;

	virtual void decompressImpl(Buffer &rawData,const Buffer &previousData,bool verify) override final;

	static bool detectHeaderXPK(uint32_t hdr) noexcept;
	static std::unique_ptr<XPKDecompressor> create(uint32_t hdr,uint32_t recursionLevel,const Buffer &packedData,std::unique_ptr<XPKDecompressor::State> &state,bool verify);

private:
	class ArithDecoder {
	public:
		ArithDecoder(ForwardInputStream &inputStream);
		~ArithDecoder();

		uint16_t decode(uint16_t length);
		void scale(uint16_t newLow,uint16_t newHigh,uint16_t newRange);

	private:
		LSBBitReader<ForwardInputStream>	_bitReader;

		uint16_t				_low=0;
		uint16_t				_high=0xffffU;
		uint16_t				_stream;
	};

	const Buffer	&_packedData;

	static XPKDecompressor::Registry<ARTMDecompressor> _XPKregistration;
};

#endif
