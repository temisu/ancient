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
	if (sizeof(size_t)==4) return value<0x1'0000'0000ULL;
		else return true;
}

#endif
