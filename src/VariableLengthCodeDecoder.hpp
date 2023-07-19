/* Copyright (C) Teemu Suutari */

#ifndef VARIABLELENGTHCODEDECODER_HPP
#define VARIABLELENGTHCODEDECODER_HPP

#include <cstddef>
#include <cstdint>

#include <array>

// For exception
#include "Decompressor.hpp"

namespace ancient::internal
{

template<size_t N>
class VariableLengthCodeDecoder
{
public:
	template<typename ...Args>
	VariableLengthCodeDecoder(Args ...args) noexcept :
		_bitLengths{uint8_t(args)...}
	{
		uint32_t length{0};
		for (uint32_t i=0;i<N;i++)
		{
			_offsets[i]=length;
			length+=1U<<_bitLengths[i];
		}
	}
	~VariableLengthCodeDecoder() noexcept=default;

	template<typename F>
	uint32_t decode(F bitReader,uint32_t base)
	{
		if (base>=N)
			throw Decompressor::DecompressionError();
		return _offsets[base]+bitReader(_bitLengths[base]);
	}

private:
	const std::array<uint8_t,N>	_bitLengths;
	std::array<uint32_t,N>		_offsets;
};

template<typename ...Args>
VariableLengthCodeDecoder(Args...args)->VariableLengthCodeDecoder<sizeof...(args)>;

}

#endif
