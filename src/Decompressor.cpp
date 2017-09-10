/* Copyright (C) Teemu Suutari */

#include "Decompressor.hpp"

std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<Decompressor>(*)(const Buffer&,bool)>> *Decompressor::_decompressors=nullptr;

Decompressor::~Decompressor()
{
	// nothing needed
}

const std::string &Decompressor::getName() const
{
	static std::string name="<invalid>";
	return name;
}

std::unique_ptr<Decompressor> Decompressor::create(const Buffer &packedData,bool exactSizeKnown)
{
	uint32_t hdr;
	if (!packedData.readBE(0,hdr)) return nullptr;
	for (auto &it : *_decompressors)
	{
		if (it.first(hdr)) return it.second(packedData,exactSizeKnown);
	}
	return nullptr;
}

void Decompressor::registerDecompressor(bool(*detect)(uint32_t),std::unique_ptr<Decompressor>(*create)(const Buffer&,bool))
{
	static std::vector<std::pair<bool(*)(uint32_t),std::unique_ptr<Decompressor>(*)(const Buffer&,bool)>> _list;
	if (!_decompressors) _decompressors=&_list;
	_decompressors->push_back(std::make_pair(detect,create));
}
