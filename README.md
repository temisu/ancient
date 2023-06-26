# Ancient - Modern decompressor for old data compression formats

This is a collection of decompression routines for old formats popular in the Amiga, Atari computers and some other systems from 80's and 90's as well as some that are currently used which were used in a some specific way in these old systems.

Even though most of these algorithms are still available for download, scavenging and using them might prove to be a challenge. Thus the purpose of this project is to:
* Provide a clean, modern implementation of the algorithms - Typically the implementations were not meant to be used outside of the original systems they were made for. Some other ported implementations are incomplete, bad quality or direct translations from old M68K assembly code.
* Provide a clean BSD-style licensing - Original implementations or their ports might have strange license or no visible license at all. There are also implementations that have been ripped off from some other source thus their legality is questionable at best.
* Provide a tested implementation - The code is no good if it does not work properly and the old code have a lot of corner cases. These implementations are tested using a cache of available files (~10k) that used these algorithms. Although it does not offer any guarantee especially when we are talking about undocumented formats, it gives hope that there are less "stupid errors" in the code. I have also generated a small batch of test files for different formats for testing. The source files are known public domain sources

For simple usage both a simple command line application as well as a simple API to use the decompressors are provided.  The compression algorithm is automatically detected in most cases, however there are some corner cases where it is not entirely reliable due to weaknesses in the old format used. Please see the [main.cpp](main.cpp) and [ancient.hpp](api/ancient/ancient.hpp) to get an idea.

This code should compile cleanly on most C++17 capable compilers, and it is tested on clang and MSVC.

Some formats have incorporated weak password protection on them which can be bypassed. However, this project does not attempt to do any real cryptograpy.

Currently the project does not support any archival files nor self extracting executables.

Decompression algorithms provided:
* bzip2
  * both normal and randomized bitstreams
* Compact (Unix)
* Compress (Unix)
  * Supports both old and new formats
* CrunchMania by Thomas Schwarz
  * CrM!: Crunch-Mania standard-mode
  * Crm!: Crunch-Mania standard-mode, sampled
  * CrM2: Crunch-Mania LZH-mode
  * Crm2: Crunch-Mania LZH-mode, sampled
* Disk Masher System a.k.a. DMS
  * Supports all different compression methods (NONE,SIMPLE,QUICK,MEDIUM,DEEP,HEAVY1,HEAVY2)
  * Supports password bypassing
* File Imploder (and most of its clones)
  * ATN!
  * BDPI
  * CHFI
  * EDAM
  * IMP!
  * M.H.
  * RDC9
  * FLT! (verification missing)
  * Dupa (verification missing)
  * PARA (verification missing)
* Freeze/Melt
  * Supports both old and new formats
* gzip
* LOB's File Compressor (Also known as a Multipak)
  * Supports all original 6 modes and their combinations (BMC, HUF, LZW, LZB, MSP, MSS)
  * Does not support mode 8 (as defined by some game files)
* Pack (Unix)
  * Supports both old and new formats
* PowerPacker
  * PP 1.1 (verification missing)
  * PP 2.0
  * PX20: Supports bypassing password protected files.
* Quasijarus Strong Compression
* Rob Northen compressors.
  * RNC1: Both old and formats utilizing the same header. heuristics for detecting the correct one
  * RNC2: RNC version 2 stream
* Turbo Packer by Wolfgang Mayerle.
* MMCMP: Music Module Compressor
* StoneCracker
  * SC: StoneCracker v2.69 - v2.81
  * SC: StoneCracker v2.92, v2.99
  * S300: StoneCracker v3.00
  * S310: StoneCracker v3.10, v3.11b
  * S400: StoneCracker pre v4.00
  * S401: StoneCracker v4.01
  * S403: StoneCracker v4.02a
  * S404: StoneCracker v4.10
* XPK-encapsulated files
  * ACCA: Andre's Code Compression Algorithm
  * ARTM: Arithmetic encoding compressor
  * BLZW: LZW-compressor
  * BZP2: Bzip2 backend for XPK
  * CBR0: RLE compressor
  * CBR1: RLE compressor
  * CRM2: CrunchMania backend for XPK
  * CRMS: CrunchMania backend for XPK, sampled
  * CYB2: xpkCybPrefs container
  * DLTA: Delta encoding
  * DUKE: NUKE with Delta encoding
  * ELZX: LZX-compressor
  * FAST: LZ77-compressor
  * FBR2: CyberYAFA compressor
  * FRHT: LZ77-compressor
  * FRLE: RLE compressor
  * GZIP: Deflate backend for XPK
  * HUFF: Huffman modeling compressor
  * HFMN: Huffman modeling compressor
  * ILZR: Incremental Lempel-Ziv-Renau compressor
  * IMPL: File Imploder backend for XPK
  * LHLB: LZRW-compressor
  * LIN1: Lino packer
  * LIN2: Lino packer
  * LIN3: Lino packer
  * LIN4: Lino packer
  * LZBS: CyberYAFA compressor
  * LZCB: LZ-compressor
  * LZW2: CyberYAFA compressor
  * LZW3: CyberYAFA compressor
  * LZW4: CyberYAFA compressor
  * LZW5: CyberYAFA compressor
  * MASH: LZRW-compressor
  * NONE: Null compressor
  * NUKE: LZ77-compressor
  * PWPK: PowerPacker backend for XPK
  * RAKE: LZ77-compressor
  * RDCN: Ross Data Compression
  * RLEN: RLE compressor
  * SASC: LZ-compressor with arithmetic encoding
  * SDHC: Sample delta huffman compressor
  * SHR3: LZ-compressor with arithmetic encoding
  * SHRI: LZ-compressor with arithmetic encoding
  * SHSC: Context modeling compressor
  * SLZ3: CyberYAFA compressor
  * SLZX: LZX-compressor with delta encoding
  * SMPL: Huffman compressor with delta encoding
  * SQSH: Compressor for sampled sounds
  * TDCS: LZ77-compressor
  * ZENO: LZW-compressor

There is some support for archival decompressors: However, these are not built in at the moment but the code can be as a reference

* Zip decompressor backend (decompressor only, no Zip file format reading yet)
  * Shrink
  * Reduce
  * Implode
  * Deflate
  * Deflate64
  * Bzip2
* Lha/Lzh decompressor backend (decompressor only, no Lha file format reading yet)
  * LH0: Null compressor
  * LH1: LZRW-compressor with 4kB window
  * LH2: LZRW-compressor with Dynamic Huffman Encoding (experimental)
  * LH3: LZRW-compressor (experimental)
  * LH4: LZRW-compressor with 4kB window
  * LH5: LZRW-compressor with 8kB window
  * LH6: LZRW-compressor with 32kB window
  * LH7: LZRW-compressor with 64kB window
  * LH8: LZRW-compressor with 64kB window (Joe Jared extension)
  * LHX: LZRW-compressor with up to 512kB window (UnLHX extension)
  * LZ4: Null compressor
  * LZ5: LZ-compressor
  * LZS: LZ-compressor
  * PM0: Null compressor
  * PM1: LZ-compressor
  * PM2: LZ-compressor

Special thanks go to Cholok for providing me references to many of the XPK-compressors.

BZIP2 tables for randomization have been included, they have BZIP2-license.

SASC/SHSC decompressors have been re-implemented by using the original HA code from Harri Hirvola as reference. (No code re-used)

Some of the rare Lzh-compressors have been re-implemented by using Lhasa as a reference. (No code re-used)


I'm slowly adding new stuff. If your favorite is not listed contact me and maybe I can add it.

Currently not planned to be supported:
* PPC only XPK compressors. XPK implementation is now considered complete in practical terms for classic Amiga.

Wishlist:
* More files for my testbench.

Feedback: tz at iki dot fi
