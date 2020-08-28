/* Copyright (C) Teemu Suutari */

#include <cstdint>

#include <memory>
#include <fstream>
#include <vector>
#include <string>
#include <functional>

#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>

#include "common/MemoryBuffer.hpp"
#include "common/SubBuffer.hpp"
#include "common/StaticBuffer.hpp"
#include "Decompressor.hpp"

std::unique_ptr<Buffer> readFile(const std::string &fileName)
{

	std::unique_ptr<Buffer> ret=std::make_unique<MemoryBuffer>(0);
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
		fprintf(stderr,"Could not read file %s\n",fileName.c_str());
		return std::make_unique<StaticBuffer<0>>();
	}
	return ret;
}

bool writeFile(const std::string &fileName,const Buffer &content)
{
	bool ret=false;
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open())
	{
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
		fprintf(stderr,"Usage: ancient identify packed_input_file\n");
		fprintf(stderr," - identifies compression used in a file\n");
		fprintf(stderr,"Usage: ancient verify packed_input_file unpacked_comparison_file\n");
		fprintf(stderr," - verifies decompression against known good unpacked file\n");
		fprintf(stderr,"Usage: ancient decompress packed_input_file output_file\n");
		fprintf(stderr," - decompresses single file\n");
		fprintf(stderr,"Usage: ancient scan input_dir output_dir\n");
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
		auto packed{readFile(argv[2])};
		std::unique_ptr<Decompressor> decompressor;
		try
		{
			decompressor=Decompressor::create(*packed,true,true);
		} catch (const Decompressor::InvalidFormatError&)
		{
			fprintf(stderr,"Unknown or invalid compression format in file %s\n",argv[2]);
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
		auto packed{readFile(argv[2])};
		std::unique_ptr<Decompressor> decompressor;
		try
		{
			decompressor=Decompressor::create(*packed,true,true);
		} catch (const Decompressor::InvalidFormatError&)
		{
			fprintf(stderr,"Unknown or invalid compression format in file %s\n",argv[2]);
			return -1;
		} catch (const Decompressor::VerificationError&)
		{
			fprintf(stderr,"Verify (packed) failed for %s\n",argv[2]);
			return -1;
		}

		std::unique_ptr<Buffer> raw;
		try
		{
			raw=std::make_unique<MemoryBuffer>((decompressor->getRawSize())?decompressor->getRawSize():Decompressor::getMaxRawSize());
		} catch (const Buffer::Error&) {
			fprintf(stderr,"Out of memory\n");
			return -1;
		}

		try
		{
			decompressor->decompress(*raw,true);
		} catch (const Decompressor::DecompressionError&)
		{
			fprintf(stderr,"Decompression failed for %s\n",argv[2]);
			return -1;
		} catch (const Decompressor::VerificationError&)
		{
			fprintf(stderr,"Verify (raw) failed for %s\n",argv[2]);
			return -1;
		}
		try
		{
			raw->resize(decompressor->getRawSize());
		} catch (const Buffer::Error&) {
			fprintf(stderr,"Out of memory\n");
			return -1;
		}

		if (decompressor->getImageOffset() || decompressor->getImageSize())
		{
			printf("File %s is disk image, decompressed stream offset is %zu, full image size is %zu\n",argv[2],decompressor->getImageOffset(),decompressor->getImageSize());
			printf("!!! Please note !!!\n!!! The destination will not be padded !!!\n\n");
		}

		if (cmd=="decompress")
		{
			writeFile(argv[3],*raw);
			return 0;
		} else {
			auto verify{readFile(argv[3])};
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
			printf("Files match!\n");
			return 0;
		}
	}
	 else if (cmd=="scan") {
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
					struct stat st;
					if (stat(name.c_str(),&st)<0) continue;
					if (st.st_mode&S_IFDIR)
					{
						processDir(name);
					} else if (st.st_mode&S_IFREG) {
						auto packed{readFile(name)};
						ConstSubBuffer scanBuffer(*packed,0,packed->size());
						for (size_t i=0;i<packed->size();)
						{
							scanBuffer.adjust(i,packed->size()-i);
							// We will detect first, before trying the format for real
							if (!Decompressor::detect(scanBuffer))
							{
								i++;
								continue;
							}
							try
							{
								auto decompressor{Decompressor::create(scanBuffer,false,true)};

								std::unique_ptr<Buffer> raw;
								try
								{
									raw=std::make_unique<MemoryBuffer>((decompressor->getRawSize())?decompressor->getRawSize():Decompressor::getMaxRawSize());
								} catch (const Buffer::Error&) {
									fprintf(stderr,"Out of memory\n");
									i++;
									continue;
								}
								// for formats that do not encode packed size.
								// we will get it from decompressor
								if (!decompressor->getPackedSize())
									decompressor->decompress(*raw,true);
								if (decompressor->getPackedSize())
								{
									// final checks with the limited buffer and fresh decompressor
									ConstSubBuffer finalBuffer(*packed,i,decompressor->getPackedSize());
									auto decompressor2{Decompressor::create(finalBuffer,true,true)};
									decompressor2->decompress(*raw,true);
									std::string outputName=std::string(argv[3])+"/file"+std::to_string(fileIndex++)+".pack";
									printf("Found compressed stream at %zu, size %zu in file %s with type '%s', storing it into %s\n",i,decompressor2->getPackedSize(),name.c_str(),decompressor2->getName().c_str(),outputName.c_str());
									writeFile(outputName,finalBuffer);
									i+=finalBuffer.size();
									continue;
								}
							} catch (const Decompressor::Error&) {
								// full steam ahead (with next offset)
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
		usage();
		return -1;
	}
}
