# ancient_format_decompressor

Decompression routines for ancient formats popular in the Amiga / Atari computers from 90's

This is a clean decompressor implementation of several old compression algorithms, where implementations
do not exists outside of the original platform, is only provided for specific processor architecture (=M68K)
or the implementations is of bad quality when it comes to error checking, completeness or
code quality (translated directly from M68K code to C)

This code should compile cleanly on any C++14 capable compiler, and it is tested on clang.

The provided tool can be used for identifying files, decompressing, verifying and scanning whole directories for
files of compressed streams. There is no support for password protected files.

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
  * DLTA: Delta encoding
  * DUKE: NUKE with Delta encoding
  * FAST: LZ77-compressor
  * FBR2: CyberYAFA compressor
  * FRLE: RLE compressor
  * GZIP: zlib backend for XPK (Deflate)
  * HUFF: Huffman modeling compressor
  * HFMN: Huffman modeling compressor
  * ILZR: Incremental Lempel-Ziv-Renau compressor
  * IMPL: File Imploder backend for XPK
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
  * SHR3: LZ-compressor with arithmetic encoding
  * SHRI: LZ-compressor with arithmetic encoding
  * SLZ3: CyberYAFA compressor
  * SMPL: Huffman compressor with delta encoding
  * SQSH: Compressor for sampled sounds
  * TDCS: LZ77-compressor
  * ZENO: LZW-compressor

Special thanks go to Cholok for providing me references to many of the compressors.

Feedback: tz at iki dot fi
