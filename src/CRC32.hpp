/* Copyright (C) Teemu Suutari */

#ifndef CRC32_HPP
#define CRC32_HPP

#include <stdint.h>

#include "Buffer.hpp"

bool CRC32(const Buffer &buffer,size_t offset,size_t len,uint32_t &accumulator);

#endif
