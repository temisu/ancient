/* Copyright (C) Teemu Suutari */

#include <memory>

#include <stdint.h>
#include <fstream>
#include <vector>
#include <string>

#include <stdio.h>
#include <dirent.h>

#include <Buffer.hpp>
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
		size_t length=size_t(file.tellg());
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
	if (argc!=4)
	{
		fprintf(stderr,"Usage: <prog> cmd input output\n");
		fprintf(stderr,"Usage: cmd=<decompress,verify>\n");
		return -1;
	}
	std::string cmd=argv[1];

	if (cmd=="decompress" || cmd=="verify")
	{
		std::unique_ptr<Buffer> packed{readFile(argv[2])};
		std::unique_ptr<Decompressor> decompressor{CreateDecompressor(*packed)};
		if (!decompressor)
		{
			printf("Unknown compression format in file %s\n",argv[2]);
			return -1;
		}
		if (!decompressor->isValid())
		{
			fprintf(stderr,"File %s is not valid or is corrupted\n",argv[2]);
			return -1;
		}
		//printf("Compression is '%s'\n",decompressor->getName().c_str());
		if (!decompressor->verifyPacked())
		{
			fprintf(stderr,"File %s has errors in packed stream\n",argv[2]);
			return -1;
		}
		std::unique_ptr<Buffer> raw{new VectorBuffer()};
		raw->resize(decompressor->getRawSize());
		if (!decompressor->decompress(*raw))
		{
			fprintf(stderr,"Decompression failed for %s\n",argv[2]);
			return -1;
		}
		if (!decompressor->verifyRaw(*raw))
		{
			fprintf(stderr,"Verify failed for %s\n",argv[2]);
			return -1;
		}
		if (cmd=="decompress")
		{
			writeFile(argv[3],*raw);
			return 0;
		} else {
			std::unique_ptr<Buffer> verify{readFile(argv[3])};
			if (raw->size()!=verify->size())
			{
				fprintf(stderr,"Verify failed for %s and %s - sizes differ\n",argv[2],argv[3]);
				return -1;
			}
			for (size_t i=0;i<raw->size();i++)
			{
				if (raw->data()[i]!=verify->data()[i])
				{
					fprintf(stderr,"Verify failed for %s and %s - contents differ @%zu\n",argv[2],argv[3],i);
					return -1;
				}
			}
			return 0;
		}
	} else {
		fprintf(stderr,"Unknown command\n");
		return -1;
	}
}
