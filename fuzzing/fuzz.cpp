/* Copyright (C) Teemu Suutari */

#include <cstdint>

#include <memory>
#include <fstream>
#include <string>

#include "common/MemoryBuffer.hpp"
#include "common/SubBuffer.hpp"
#include "common/StaticBuffer.hpp"
#include "Decompressor.hpp"

using namespace ancient::internal;

std::shared_ptr<Buffer> readFile(const std::string &fileName)
{

	std::shared_ptr<Buffer> ret=std::make_shared<MemoryBuffer>(0);
	std::ifstream file(fileName.c_str(),std::ios::in|std::ios::binary);
	bool success=false;
	if (file.is_open())
	{
		file.seekg(0,std::ios::end);
		size_t length=size_t(file.tellg());
		file.seekg(0,std::ios::beg);
		ret->resize(length);
		file.read(reinterpret_cast<char*>(ret->data()),length);
		success=bool(file);
		file.close();
	}
	if (!success)
	{
		return {};
	}
	return ret;
}

int main(int argc,char **argv)
{
	(void)argc;
#ifdef __AFL_HAVE_MANUAL_CONTROL
	__AFL_INIT();
#endif
	
	auto packed{readFile(argv[1])};
	if (!packed)
	{
		return -1;
	}
	std::shared_ptr<Decompressor> decompressor;
	try
	{
		decompressor=Decompressor::create(*packed,true,true);
	} catch (const Decompressor::InvalidFormatError&)
	{
		return -1;
	} catch (const Decompressor::VerificationError&)
	{
		return -1;
	}

	std::shared_ptr<Buffer> raw;
	try
	{
		raw=std::make_shared<MemoryBuffer>((decompressor->getRawSize())?decompressor->getRawSize():Decompressor::getMaxRawSize());
	} catch (const Buffer::Error&) {
		return -1;
	}
		
	try
	{
		decompressor->decompress(*raw,true);
	} catch (const Decompressor::DecompressionError&)
	{
		return -1;
	} catch (const Decompressor::VerificationError&)
	{
		return -1;
	}
	try
	{
		raw->resize(decompressor->getRawSize());
	} catch (const Buffer::Error&) {
		return -1;
	}

	if (decompressor->getImageOffset() || decompressor->getImageSize())
	{
		return -1;
	}

	return 0;
}
