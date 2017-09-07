# Ancient Format Decompressor

This is a collection of decompression routines for ancient formats popular in the Amiga, Atari computers (and others) from 80's and 90's as well as including some current ones that were used in a some specific way in these systems.

For simple usage both a simple command line application as well as a simple API to use the decompressors are provided. The compression algorithm is automatically detected in most cases, however there are some corner cases where it is not entirely reliable due to weaknesses in the old format used. Please see the main.cpp and Decompressor.hpp to get an idea.

Even though most of these algorithms are still available for download, scavenging and using them might prove to be a challenge. Thus the purpose of this project is to:
* Provide a clean, modern implementation of the algorithms - Typically the implementations were not meant to be used outside of the original systems they were made for. Some other ported implementations are incomplete, bad quality or direct translations from old M68K assembly code.
* Provide a clean BSD-style licensing - Original implementations or their ports might have strange license or no visible license at all. There are also implementations that have been ripped off from some other source thus their legality is questionable at best.
* Provide a tested implementation - The code is no good if it does not work properly and the old code has a lot of corner cases. The implementation is tested using the cache of available files (~10k) that used these algorithms. Although it does not offer any guarantee especially when we are talking about undocumented formats, it gives hope that there are less "stupid errors" in the code.

This code should compile cleanly on any C++14 capable compiler, and it is tested on clang. Since it does not use lot of C++14 features, you might get have luck compiling it with C++11 capable compiler if it has some rudimentary C++14 support.

Currently there are no plans to add password protected file support nor any kind of decryption capability. (apart from some very basic password bypassing in some formats that can be done easily)

Decompression algorithms provided:
* CrunchMania by Thomas Schwarz: supports both new and old stream formats. Also supports decoding the delta encoding i.e. "sampled mode"
* File Imploder (and most of its clones)
* PowerPacker
* Rob Northen compressors. supports all 3 modes.
* Turbo Packer by Wolfgang Mayerle.
* Standard gzip (only ancient in context of XPK though)
* Supports opening XPK-encapsulated files. XPK decompressors supported are:
  * ACCA: Andre's Code Compression Algorithm
  * BLZW: LZW-compressor
  * CBR0: RLE compressor
  * CRM2: CrunchMania backend for XPK
  * CRMS: CrunchMania backend for XPK (sampled)
  * CYB2: xpkCybPrefs container
  * DLTA: Delta encoding
  * DUKE: NUKE with Delta encoding
  * ELZX: LZX-compressor
  * FAST: LZ77-compressor
  * FBR2: CyberYAFA compressor
  * FRHT: LZ77-compressor
  * FRLE: RLE compressor
  * GZIP: zlib backend for XPK (Deflate)
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
  * SDHC: Sample delta huffman compressor
  * SHR3: LZ-compressor with arithmetic encoding
  * SHRI: LZ-compressor with arithmetic encoding
  * SLZ3: CyberYAFA compressor
  * SLZX: LZX-compressor with delta encoding
  * SMPL: Huffman compressor with delta encoding
  * SQSH: Compressor for sampled sounds
  * TDCS: LZ77-compressor
  * ZENO: LZW-compressor

Special thanks go to Cholok for providing me references to many of the XPK-compressors.

I'm slowly adding new stuff. If your favorite is not listed contact me and maybe I can add it.

Feedback: tz at iki dot fi
