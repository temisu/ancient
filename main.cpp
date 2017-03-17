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
	bool success=false;
	if (file.is_open())
	{
		file.seekg(0,std::ios::end);
		size_t length=size_t(file.tellg());
		file.seekg(0,std::ios::beg);
		ret->resize(length);
		file.read(reinterpret_cast<char*>(ret->data()),length);
		success=bool(file);
		if (!success) ret->resize(0);
		file.close();
	}
	if (!success)
	{
		fprintf(stderr,"Could not read file %s\n",fileName.c_str());
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
	if (!ret)
	{
		fprintf(stderr,"Could not write file %s\n",fileName.c_str());
	}
	return ret;
}

int main(int argc,char **argv)
{
	auto usage=[]()
	{
		fprintf(stderr,"Usage: <prog> indentify input_packed\n");
		fprintf(stderr," - identifies compression used in a file\n");
		fprintf(stderr,"Usage: <prog> verify input_packed input_unpacked\n");
		fprintf(stderr," - verifies decompression against known good unpacked file\n");
		fprintf(stderr,"Usage: <prog> decompress input_packed output_raw\n");
		fprintf(stderr," - decompresses single file\n");
		fprintf(stderr,"Usage: <prog> scan input_dir output_dir\n");
		fprintf(stderr," - scans input directory recursively and stores all found\n"
			       " - known compressed streams to separate files in output directory\n");
	};

	if (argc<3)
	{
		usage();
		return -1;
	}
	std::string cmd=argv[1];

	if (cmd=="identify")
	{
		if (argc!=3)
		{
			usage();
			return -1;
		}
		std::unique_ptr<Buffer> packed{readFile(argv[2])};
		std::unique_ptr<Decompressor> decompressor{CreateDecompressor(*packed)};
		if (!decompressor)
		{
			fprintf(stderr,"Unknown compression format in file %s\n",argv[2]);
			return -1;
		}
		if (!decompressor->isValid())
		{
			fprintf(stderr,"File %s is not valid or is corrupted\n",argv[2]);
			return -1;
		}
		printf("Compression of %s is %s\n",argv[2],decompressor->getName().c_str());
		return 0;
	} else if (cmd=="decompress" || cmd=="verify") {
		if (argc!=4)
		{
			usage();
			return -1;
		}
		std::unique_ptr<Buffer> packed{readFile(argv[2])};
		std::unique_ptr<Decompressor> decompressor{CreateDecompressor(*packed)};
		if (!decompressor)
		{
			fprintf(stderr,"Unknown compression format in file %s\n",argv[2]);
			return -1;
		}
		if (!decompressor->isValid())
		{
			fprintf(stderr,"File %s is not valid or is corrupted\n",argv[2]);
			return -1;
		}
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
					fprintf(stderr,"Verify failed for %s and %s - contents differ @ %zu\n",argv[2],argv[3],i);
					return -1;
				}
			}
			return 0;
		}
	} else if (cmd=="scan") {
		if (argc!=4)
		{
			usage();
			return -1;
		}
		uint32_t fileIndex=0;
		std::function<void(std::string)> processDir=[&](std::string inputDir)
		{

			std::unique_ptr<DIR,decltype(&::closedir)> dir{::opendir(inputDir.c_str()),::closedir};
			if (dir)
			{
				while (struct dirent *de=::readdir(dir.get()))
				{
					std::string subName(de->d_name);
					if (subName=="." || subName=="..") continue;
					std::string name=inputDir+"/"+subName;
					if (de->d_type==DT_DIR)
					{
						processDir(name);
					} else if (de->d_type==DT_REG) {
						std::unique_ptr<Buffer> packed{readFile(name)};
						ConstSubBuffer scanBuffer(*packed,0,packed->size());
						for (size_t i=0;i<packed->size();)
						{
							if (!scanBuffer.adjust(i,packed->size()-i)) break;
							std::unique_ptr<Decompressor> decompressor{CreateDecompressor(scanBuffer)};
							if (decompressor && decompressor->isValid() && decompressor->verifyPacked())
							{
								std::unique_ptr<Buffer> raw{new VectorBuffer()};
								raw->resize((decompressor->getRawSize())?decompressor->getRawSize():Decompressor::getMaxRawSize());
								bool success=true;
								if (!decompressor->getPackedSize())
								{
									// for formats that do not encode packed size.
									// we will get it from decompressor
									success=decompressor->decompress(*raw);
								}
								if (success && decompressor->getPackedSize())
								{
									// final checks with the limited buffer and fresh decompressor
									ConstSubBuffer finalBuffer(*packed,i,decompressor->getPackedSize());
									std::unique_ptr<Decompressor> decompressor2{CreateDecompressor(finalBuffer)};
									if (decompressor2 && decompressor2->isValid() && decompressor2->verifyPacked() && decompressor2->decompress(*raw) && decompressor2->verifyRaw(*raw))
									{
										std::string outputName=std::string(argv[3])+"/file"+std::to_string(fileIndex++)+".pack";
										printf("Found compressed stream at %zu, size %zu in file %s with type '%s', storing it into %s\n",i,decompressor2->getPackedSize(),name.c_str(),decompressor2->getName().c_str(),outputName.c_str());
										writeFile(outputName,finalBuffer);
										i+=finalBuffer.size();
										continue;
									}
								}
							}
							i++;
						}
					}
				}
			} else {
				fprintf(stderr,"Could not process directory %s\n",inputDir.c_str());
			}
		};

		processDir(std::string(argv[2]));
		return 0;
	} else {
		fprintf(stderr,"Unknown command\n");
		return -1;
	}
}
