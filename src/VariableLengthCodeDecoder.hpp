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
	// negative lengths can be used to reset the offset
	template<typename ...Args>
	VariableLengthCodeDecoder(Args ...args) noexcept :
		_bitLengths{createBitLength(args)...}
	{
		// Probably this could be someway done as a nice constexpr initializer list, but I can't
		// see an easy way. Let it be for now
		uint32_t length{0};
		uint32_t i{0};

		auto foldOffsets=[&](auto value) noexcept
		{
			if (value<0)
			{
				_offsets[i]=0;
				length=1U<<-value;
			} else {
				_offsets[i]=length;
				length+=1U<<value;
			}
			i++;
		};

		(foldOffsets(args),...);
	}
	~VariableLengthCodeDecoder() noexcept=default;

	template<typename F>
	uint32_t decode(F bitReader,uint32_t base) const
	{
		if (base>=N)
			throw Decompressor::DecompressionError();
		return _offsets[base]+bitReader(_bitLengths[base]);
	}

	constexpr bool isMax(uint32_t base,uint32_t value)
	{
		if (base>=N)
			throw Decompressor::DecompressionError();
		uint32_t tmp{value-_offsets[base]};
		return tmp==(1U<<_bitLengths[base])-1U;
	}

private:
	template<typename T>
	static constexpr uint8_t createBitLength(T value) noexcept
	{
		return value>=0?value:-value;
	}

	const std::array<uint8_t,N>	_bitLengths;
	std::array<uint32_t,N>		_offsets;
};

template<typename ...Args>
VariableLengthCodeDecoder(Args...args)->VariableLengthCodeDecoder<sizeof...(args)>;

}

#endif
