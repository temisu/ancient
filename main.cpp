/* Copyright (C) Teemu Suutari */

#include <memory>

#include <stdint.h>
#include <fstream>
#include <vector>
#include <string>

#include <stdio.h>
#include <dirent.h>

#include "Buffer.hpp"
#include "Decompressor.hpp"

class VectorBuffer : public Buffer
{
public:
	VectorBuffer();

	virtual ~VectorBuffer() override final;

	virtual const uint8_t *data() const override final;
	virtual uint8_t *data() override final;
	virtual size_t size() const override final;

	virtual bool isResizable() const override final;
	virtual void resize(size_t newSize) override final;

private:
	std::vector<uint8_t>  _data;
};

VectorBuffer::VectorBuffer()
{
	// nothing needed
}

VectorBuffer::~VectorBuffer()
{
	// nothing needed
}

const uint8_t *VectorBuffer::data() const
{
	return _data.data();
}

uint8_t *VectorBuffer::data()
{
	return _data.data();
}

size_t VectorBuffer::size() const
{
	return _data.size();
}

bool VectorBuffer::isResizable() const
{
	return true;
}

void VectorBuffer::resize(size_t newSize) 
{
	return _data.resize(newSize);
}

Buffer *readFile(const std::string &fileName)
{

	Buffer *ret=new VectorBuffer();
	std::ifstream file(fileName.c_str(),std::ios::in|std::ios::binary);
	if (file.is_open())
	{
		file.seekg(0,std::ios::end);
		size_t length=file.tellg();
		file.seekg(0,std::ios::beg);
		ret->resize(length);
		file.read(reinterpret_cast<char*>(ret->data()),length);
		if (!bool(file)) ret->resize(0);
		file.close();
	}
	return ret;
}

bool writeFile(const std::string &fileName,const Buffer &content)
{
	bool ret=false;
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open()) {
		file.write(reinterpret_cast<const char*>(content.data()),content.size());
		ret=bool(file);
		file.close();
	}
	return ret;
}

int main(int argc,char **argv)
{
	if (argc!=3)
	{
		printf("Usage: <prog> infile outfile\n");
		return -1;
	}
	std::unique_ptr<Buffer> packed{readFile(argv[1])};
	std::unique_ptr<Decompressor> decompressor{CreateDecompressor(*packed)};
	if (!decompressor)
	{
		printf("Unknown compression format in file %s\n",argv[1]);
		return -1;
	}
	if (!decompressor->isValid())
	{
		printf("File %s is not valid or is corrupted\n",argv[1]);
		return -1;
	}
	if (!decompressor->verifyPacked())
	{
		printf("File %s has errors in packed stream\n",argv[1]);
		return -1;
	}
	std::unique_ptr<Buffer> raw{new VectorBuffer()};
	raw->resize(decompressor->getRawSize());
	if (!decompressor->decompress(*raw))
	{
		printf("Decompression failed for %s\n",argv[1]);
		return -1;
	}
	if (!decompressor->verifyRaw(*raw))
	{
		printf("Verify failed for %s\n",argv[1]);
		return -1;
	}
	writeFile(argv[2],*raw);
	return 0;
}
