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

void verifyFile(const char *packedFile,const std::vector<uint8_t> &verify,bool ignoreExpansion=false)
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
	if ((verify.size()!=actualSize && !ignoreExpansion) ||
		(actualSize<verify.size() && ignoreExpansion))
	{
		fprintf(stderr,"Verify failed for %s - size differs from original\n",packedFile);
		exit(1);
	}
	size_t offset=decompressor->getImageOffset()?decompressor->getImageOffset().value():0;
	for (size_t i=0;i<verify.size();i++)
	{
		if (raw.data()[i]!=verify.data()[i+offset])
		{
			fprintf(stderr,"Verify failed for %s - contents differ @ %zu from original\n",packedFile,i);
			exit(1);
		}
	}
}

void verifyFile(const char *packedFile,const char *rawFile,bool ignoreExpansion=false)
{
	auto verify{readFile(rawFile)};
	verifyFile(packedFile,*verify,ignoreExpansion);
}

#define BASE_DIR "testing/test_files/"

int main(int argc,char **argv)
{
	// Bzip2
	verifyFile(BASE_DIR "test_C1.bz2",BASE_DIR "test_C1.raw");
	{
		// not going to check in 1M of zeros (randomisation test)
		std::vector<uint8_t> empty(1024*1024);
		for (uint32_t i=0;i<empty.size();i++) empty[i]=0; // probably redudant
		verifyFile(BASE_DIR "test_r1.bz2",empty);
	}

	// Compact
	verifyFile(BASE_DIR "test_C1.C",BASE_DIR "test_C1.raw");

	// Compress
	verifyFile(BASE_DIR "test_C1_old.Z",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.Z",BASE_DIR "test_C1.raw");

	// Crunch-Mania
	verifyFile(BASE_DIR "test_C1.crm",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_delta.crm",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lz.crm",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lz_delta.crm",BASE_DIR "test_C1.raw");

	// DMS
	verifyFile(BASE_DIR "test_C1_none.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_none_pwd.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_simple.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_simple_pwd.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_quick.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_quick_pwd.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_medium.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_medium_pwd.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_deep.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_deep_pwd.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_heavy1.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_heavy1_pwd.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_heavy2.dms",BASE_DIR "test_C1.adf");
	verifyFile(BASE_DIR "test_C1_heavy2_pwd.dms",BASE_DIR "test_C1.adf");
	// One HD disk as well
	verifyFile(BASE_DIR "test_C1_ext.dms",BASE_DIR "test_C1_ext.adf");

	// File Imploder
	verifyFile(BASE_DIR "test_C1.imp",BASE_DIR "test_C1.raw");

	// Freeze
	verifyFile(BASE_DIR "test_C1_old.F",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.F",BASE_DIR "test_C1.raw");

	// Gzip
	verifyFile(BASE_DIR "test_C1.gz",BASE_DIR "test_C1.raw");

	// LOB
	verifyFile(BASE_DIR "test_C1_m1.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m2.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m3.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m4.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m5.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m6.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m7.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m8.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m9.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m10.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m11.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m12.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m13.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m14.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m15.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m16.lob",BASE_DIR "test_C1.raw",true);
	verifyFile(BASE_DIR "test_C1_m17.lob",BASE_DIR "test_C1.raw",true);

	// TODO: MMCMP missing
	verifyFile(BASE_DIR "test_C2.mmcmp122",BASE_DIR "test_C2.xm",true);
	verifyFile(BASE_DIR "test_C2.mmcmp130",BASE_DIR "test_C2.xm",true);
	verifyFile(BASE_DIR "test_C2.mmcmp132",BASE_DIR "test_C2.xm",true);
	verifyFile(BASE_DIR "test_C2.mmcmp134",BASE_DIR "test_C2.xm",true);

	// Pack
	verifyFile(BASE_DIR "test_C1_pack.z",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_pack_old.z",BASE_DIR "test_C1.raw");

	// Powerpacker
	verifyFile(BASE_DIR "test_C1.pp",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_px20.pp",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_px20_b.pp",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_px20_c.pp",BASE_DIR "test_C1.raw");

	// Quasijarus strong compression
	verifyFile(BASE_DIR "test_C1_q.Z",BASE_DIR "test_C1.raw");

	// RNC
	verifyFile(BASE_DIR "test_C1.rnc1old",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.rnc1",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1.rnc2",BASE_DIR "test_C1.raw");

	// TODO: SCO Compress LZH
	// I'm not going to buy a openserver license to compress files
	// I hope someone can help me here

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

	// TPWM
	verifyFile(BASE_DIR "test_C1.tpwm",BASE_DIR "test_C1.raw");

	// XPK
	verifyFile(BASE_DIR "test_C1_acca.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_artm.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_blzw.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_bzp2.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_cbr0.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C2_cbr0.xpkf",BASE_DIR "test_C2.xm"); // CBR0 does not really want to compress alice
	verifyFile(BASE_DIR "test_C1_crm2.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_crms.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_cyb2.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_dlta.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_duke.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_elzx.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_fast.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_fbr2.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_frht.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_frle.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_gzip.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_hfmn.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_huff.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_ilzr.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_impl.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lhlb.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lin1.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lin2.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lin3.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lin4.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lzbs.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lzcb.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lzw2.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lzw3.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lzw4.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_lzw5.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_mash.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_none.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_nuke.xpkf",BASE_DIR "test_C1.raw");
//	verifyFile(BASE_DIR "test_C1_ppmq.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_pwpk.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_rake.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_rdcn.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_rlen.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_sasc.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_sdhc.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_shr3.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_shri.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_shsc.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_slz3.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_slzx.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_smpl.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_sqsh.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_tdcs.xpkf",BASE_DIR "test_C1.raw");
	verifyFile(BASE_DIR "test_C1_zeno.xpkf",BASE_DIR "test_C1.raw");

	return 0;
}
