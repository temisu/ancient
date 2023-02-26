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

void verifyFile(const char *packedFile,const char *rawFile,bool ignoreExpansion=false)
{
	auto packed{readFile(packedFile)};
	std::optional<ancient::Decompressor> decompressor;
	try
	{
		decompressor.emplace(*packed,true,true);
	} catch (const ancient::InvalidFormatError&)
	{
		fprintf(stderr,"Unknown or invalid compression format in file %s\n",packedFile);
		exit(1);
	} catch (const ancient::VerificationError&)
	{
		fprintf(stderr,"Verify (packed) failed for %s\n",packedFile);
		exit(1);
	}

	std::vector<uint8_t> raw;
	try
	{
		raw=decompressor->decompress(true);
	} catch (const ancient::DecompressionError&)
	{
		fprintf(stderr,"Decompression failed for %s\n",packedFile);
		exit(1);
	} catch (const ancient::VerificationError&)
	{
		fprintf(stderr,"Verify (raw) failed for %s\n",packedFile);
		exit(1);
	} catch (const std::bad_alloc&) {
		fprintf(stderr,"Out of memory\n");
		exit(1);
	}

	size_t actualSize=decompressor->getImageSize()?decompressor->getImageSize().value():raw.size();
	auto verify{readFile(rawFile)};
	if ((verify->size()!=actualSize && !ignoreExpansion) ||
		(actualSize<verify->size() && ignoreExpansion))
	{
		fprintf(stderr,"Verify failed for %s and %s - sizes differ\n",packedFile,rawFile);
		exit(1);
	}
	size_t offset=decompressor->getImageOffset()?decompressor->getImageOffset().value():0;
	for (size_t i=0;i<verify->size();i++)
	{
		if (raw.data()[i]!=verify->data()[i+offset])
		{
			fprintf(stderr,"Verify failed for %s and %s - contents differ @ %zu\n",packedFile,rawFile,i);
			exit(1);
		}
	}
}

#define BASE_DIR "testing/test_files/"

int main(int argc,char **argv)
{
	// Compress
	verifyFile(BASE_DIR "test_C1_old.Z",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.Z",BASE_DIR "test_C1.raw");

	// Freeze
	verifyFile(BASE_DIR "test_C1_old.F",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.F",BASE_DIR "test_C1.raw");

	// Pack
	verifyFile(BASE_DIR "test_C1_pack.z",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_pack_old.z",BASE_DIR "test_C1.raw");

	// Powerpacker
	verifyFile(BASE_DIR "test_C1.pp",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_px20.pp",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_px20_b.pp",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_px20_c.pp",BASE_DIR "test_C1.raw");

	// RNC
	// TODO: missing old RNC1 and RNC2
	verifyFile(BASE_DIR "test_C1.rnc1",BASE_DIR "test_C1.raw");

	// Stonecracker
	verifyFile(BASE_DIR "test_C1.pack271_000",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_032",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_033",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_132",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_232",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_332",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_432",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_532",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_632",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_732",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack271_733",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_0",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_1",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_2",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_3",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_4",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_5",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_6",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack292_7",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack299_0",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack300_0",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack310",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1.pack401",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack402",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack410",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.pack410_0",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.packpre400",BASE_DIR "test_C1.raw");

	// XPK
	verifyFile(BASE_DIR "test_C1_xpkacca.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkartm.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkblzw.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkbzp2.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkcbr0.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkcrm2.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkcrms.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkcyb2.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkdlta.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkduke.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkelzx.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkfast.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkfbr2.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkfrht.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkfrle.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkgzip.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkhfmn.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkhuff.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkilzr.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkimpl.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklhlb.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklin1.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklin2.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklin3.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklin4.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklzbs.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklzcb.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklzw2.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklzw3.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklzw4.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpklzw5.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkmash.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpknone.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpknuke.pack",BASE_DIR "test_C1.raw");
//	verifyFile(BASE_DIR "test_C1_xpkppmq.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkpwpk.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkrake.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkrdcn.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkrlen.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpksasc.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpksdhc.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkshr3.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkshri.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkshsc.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkslz3.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkslzx.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpksmpl.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpksqsh.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpktdcs.pack",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_xpkzeno.pack",BASE_DIR "test_C1.raw");

//	verifyFile(BASE_DIR "",BASE_DIR "test_C1.raw");
}
