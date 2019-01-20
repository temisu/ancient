/* Copyright (C) Teemu Suutari */

#ifndef CRC16_HPP
#define CRC16_HPP

#include <stdint.h>

#include "Buffer.hpp"

// The most common CRC16

uint16_t CRC16(const Buffer &buffer,size_t offset,size_t len,uint16_t accumulator);

uint16_t CRC16Byte(uint8_t ch,uint16_t accumulator) noexcept;

#endif
