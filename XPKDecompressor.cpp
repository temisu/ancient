/* Copyright (C) Teemu Suutari */

#include "XPKDecompressor.hpp"

XPKDecompressor::State::~State()
{
	// nothing needed
}

XPKDecompressor::~XPKDecompressor()
{
	// nothing needed
}

const std::string &XPKDecompressor::getSubName() const
{
	static std::string name="<invalid>";
	return name;
}
