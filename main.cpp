/* Copyright (C) Teemu Suutari */

#include <fstream>
#include <functional>
#include <new>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>

#include <sys/stat.h>

#include <ancient/ancient.hpp>

// enables scanning. Useful for testing/debugging. Not so useful otherwise
// Not good for default since creates dependency for dirent (i.e. windows blues)
//#define ENABLE_SCAN 1

#ifdef ENABLE_SCAN
#include <dirent.h>
#endif

std::unique_ptr<std::vector<uint8_t>> readFile(const std::string &fileName)
{
	std::unique_ptr<std::vector<uint8_t>> ret=std::make_unique<std::vector<uint8_t>>();
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
		return std::make_unique<std::vector<uint8_t>>();
	}
	return ret;
}

bool writeFile(const std::string &fileName,const uint8_t *data, size_t size)
{
	bool ret=false;
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open())
	{
		file.write(reinterpret_cast<const char*>(data),size);
		ret=bool(file);
		file.close();
	}
	if (!ret)
	{
		fprintf(stderr,"Could not write file %s\n",fileName.c_str());
	}
	return ret;
}

bool writeFile(const std::string &fileName,const std::vector<uint8_t> &content)
{
	return writeFile(fileName,content.data(),content.size());
}

int main(int argc,char **argv)
{
	auto usage=[]()
	{
		fprintf(stderr, "Ancient v2.1.0\n"
				"Copyright (C) Teemu Suutari\n"
				"\n"
				"Usage: ancient i[dentify] packed_input_files...\n"
				" - identifies compression used in a file(s)\n"
				"Usage: ancient v[erify] packed_input_file unpacked_comparison_file\n"
				" - verifies decompression against known good unpacked file\n"
				"Usage: ancient d[ecompress] packed_input_file output_file\n"
				" - decompresses single file\n");
#ifdef ENABLE_SCAN
		fprintf(stderr,	"Usage: ancient s[can] input_dir output_dir\n"
				" - scans input directory recursively and stores all found\n"
				" - known compressed streams to separate files in output directory\n");
#endif
	};

	if (argc<3)
	{
		usage();
		return -1;
	}
	std::string cmd=argv[1];

	if (cmd=="i" || cmd=="identify")
	{
		if (argc<3)
		{
			usage();
			return -1;
		}
		for (int i=2;i<argc;i++)
		{
			auto packed{readFile(argv[i])};
			std::optional<ancient::Decompressor> decompressor;
			try
			{
				decompressor.emplace(*packed,true,true);
				printf("Compression of %s is %s\n",argv[i],decompressor->getName().c_str());
			} catch (const ancient::InvalidFormatError&)
			{
				fprintf(stderr,"Unknown or invalid compression format in file %s\n",argv[i]);
			} catch (const ancient::VerificationError&)
			{
				fprintf(stderr,"Failed to validate file %s\n",argv[i]);
			}
		}
		return 0;
	} else if (cmd=="d" || cmd=="decompress" || cmd=="v" || cmd=="verify") {
		if (argc!=4)
		{
			usage();
			return -1;
		}
		auto packed{readFile(argv[2])};
		std::optional<ancient::Decompressor> decompressor;
		try
		{
			decompressor.emplace(*packed,true,true);
		} catch (const ancient::InvalidFormatError&)
		{
			fprintf(stderr,"Unknown or invalid compression format in file %s\n",argv[2]);
			return -1;
		} catch (const ancient::VerificationError&)
		{
			fprintf(stderr,"Verify (packed) failed for %s\n",argv[2]);
			return -1;
		}

		std::vector<uint8_t> raw;
		try
		{
			raw=decompressor->decompress(true);
		} catch (const ancient::DecompressionError&)
		{
			fprintf(stderr,"Decompression failed for %s\n",argv[2]);
			return -1;
		} catch (const ancient::VerificationError&)
		{
			fprintf(stderr,"Verify (raw) failed for %s\n",argv[2]);
			return -1;
		} catch (const std::bad_alloc&) {
			fprintf(stderr,"Out of memory\n");
			return -1;
		}

		if (cmd=="d" || cmd=="decompress")
		{
			if (decompressor->getImageOffset() || decompressor->getImageSize())
			{
				printf("File %s is disk image, decompressed stream offset is %zu, full image size is %zu, stream size is %zu\n",argv[2],decompressor->getImageOffset().value(),decompressor->getImageSize().value(),decompressor->getRawSize().value());
				printf("!!! Please note !!!\n!!! The destination will be padded !!!\n\n");
			}
			if (decompressor->getImageSize())
			{
				std::vector<uint8_t> pad(decompressor->getImageSize().value());
				std::memcpy(&pad[decompressor->getImageOffset()?decompressor->getImageOffset().value():0],raw.data(),raw.size());
				writeFile(argv[3],pad);
			} else {
				writeFile(argv[3],raw);
			}
			return 0;
		} else {
			size_t actualSize=decompressor->getImageSize()?decompressor->getImageSize().value():raw.size();
			auto verify{readFile(argv[3])};
			if (verify->size()!=actualSize)
			{
				fprintf(stderr,"Verify failed for %s and %s - sizes differ\n",argv[2],argv[3]);
				return -1;
			}
			size_t offset=decompressor->getImageOffset()?decompressor->getImageOffset().value():0;
			for (size_t i=0;i<raw.size();i++)
			{
				if (raw.data()[i]!=verify->data()[i+offset])
				{
					fprintf(stderr,"Verify failed for %s and %s - contents differ @ %zu\n",argv[2],argv[3],i);
					return -1;
				}
			}
			printf("Files match!\n");
			return 0;
		}
	}
#ifdef ENABLE_SCAN
	else if (cmd=="s" || cmd=="scan") {
		if (argc!=4)
		{
			usage();
			return -1;
		}
		uint32_t fileIndex=0;
		std::function<void(std::string)> processDir=[&](std::string inputDir)
		{
			auto opendir=[](const char *f)->DIR* {
				return ::opendir(f);
			};

			auto closedir=[](DIR *d)->int {
				return ::closedir(d);
			};

			std::unique_ptr<DIR,decltype(closedir)> dir{opendir(inputDir.c_str()),closedir};
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
						size_t scanPos=0;
						size_t scanSize=packed->size();
						for (size_t i=0;i<packed->size();)
						{
							scanPos+=1;
							scanSize-=1;
							// We will detect first, before trying the format for real
							if (!ancient::Decompressor::detect(packed->data()+scanPos,scanSize))
							{
								i++;
								continue;
							}
							try
							{
								ancient::Decompressor decompressor{packed->data()+scanPos,scanSize,false,true};
								// for formats that do not encode packed size.
								// we will get it from decompressor
								if (!decompressor.getPackedSize())
								{
									try
									{
										decompressor.decompress(true);
									} catch (const std::bad_alloc&) {
										fprintf(stderr,"Out of memory\n");
										i++;
										continue;
									}
								}
								if (decompressor.getPackedSize())
								{
									// final checks with the limited buffer and fresh decompressor
									const uint8_t *finalData=packed->data()+scanPos;
									size_t finalSize=decompressor.getPackedSize().value();
									ancient::Decompressor decompressor2{finalData,finalSize,true,true};
									try
									{
										decompressor2.decompress(true);
									} catch (const std::bad_alloc&) {
										fprintf(stderr,"Out of memory\n");
										i++;
										continue;
									}
									std::string outputName=std::string(argv[3])+"/file"+std::to_string(fileIndex++)+".pack";
									printf("Found compressed stream at %zu, size %zu in file %s with type '%s', storing it into %s\n",i,decompressor2.getPackedSize().value(),name.c_str(),decompressor2.getName().c_str(),outputName.c_str());
									writeFile(outputName,finalData,finalSize);
									i+=finalSize;
									continue;
								}
							} catch (const ancient::Error&) {
								// full steam ahead (with next offset)
							} catch (const std::bad_alloc&) {
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
	}
#endif
	else {
		fprintf(stderr,"Unknown command\n");
		usage();
		return -1;
	}
}
