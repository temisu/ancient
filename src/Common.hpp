/* Copyright (C) Teemu Suutari */

#ifndef COMMON_HPP
#define COMMON_HPP

#include <stdint.h>

#include <string>
#include <vector>

constexpr uint32_t FourCC(uint32_t cc) noexcept
{
	// return with something else if behavior is not same as in clang/gcc for multibyte literals
	return cc;
}

constexpr bool isValidSize(uint64_t &value) noexcept
{
#if INTPTR_MAX == INT32_MAX
	return value<0x1'0000'0000ULL;
#else
	return true;
#endif
}

constexpr bool isValidSize(off_t &value) noexcept
{
#if INTPTR_MAX == INT32_MAX
	return value<0x1'0000'0000ULL;
#else
	return true;
#endif
}

uint32_t rotateBits(uint32_t value,uint32_t count);

#endif
