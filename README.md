# ancient_format_decompressor

Decompression routines for ancient formats popular in the Amiga / Atari computers from 90's

This is a clean decompressor implementation of several old compression algorithms, where implementations
do not exists outside of the original platform, is only provided for specific processor architecture (=M68K)
or the implementations is of bad quality when it comes to error checking, completeness or
code quality (translated directly from M68K code to C)

This code should compile cleanly on any C++14 capable compiler, and it is tested on clang.

The provided tool can be used for identifying files, decompressing, verifying and scanning whole directories for
files of compressed streams.

Decompression algorithms provided:
* CrunchMania by Thomas Schwarz: supports both new and old stream formats. Also supports decoding the delta encoding i.e. "sampled mode"
* File Imploder (and most of its clones)
* PowerPacker
* Rob Northen compressors. supports all 3 modes.
* Turbo Packer by Wolfgang Mayerle.
* Standard gzip
* Supports opening XPK-encapsulated files. XPK decompressors supported are:
  * CBR0: RLE compressor
  * CRM2: CrunchMania backend for XPK
  * CRMS: CrunchMania backend for XPK (sampled)
  * DLTA: Delta encoding
  * DUKE: NUKE with Delta encoding
  * FAST: FAST LZ77 compressor
  * FRLE: RLE compressor
  * GZIP: gzip backend for XPK
  * HUFF: Huffman modeling compressor
  * HFMN: Huffman modeling compressor
  * IMPL: File Imploder backend for XPK
  * MASH: MASK LZRW-compressor
  * NONE: Null compressor
  * NUKE: NUKE LZ77-compressor
  * PWPK: PowerPacker backend for XPK
  * RLEN: RLE compressor
  * SQSH: SQSH compressor for sampled sounds

Some decompressors have tested better than others. Support for data verification is severely lacking in most formats.
Also there is not support for password protected files.

Special thanks go to Cholok for providing me references for many of the compressors with sources.

Feedback: tz at iki dot fi
