# ancient_format_decompressor

Decompression routines for ancient formats popular in the Amiga / Atari computers from 90's

This is a clean decompressor implementation of several old compression algorithms, where implementations
do not exists outside of the original platform, is only provided for specific processor architecture (=M68K)
or the implementations is of bad quality when it comes to error checking, completeness or
code quality (translated directly from M68K code to C)

This code should compile cleanly on any C++14 capable compiler, and it is tested on clang.

Decompression algorithms provided:
- Crunch Mania by Thomas Schwarz: supports both new and old stream formats. Also supports decoding the
  delta encoding i.e. "sampled mode"
- File Imploder (and most of its clones)
- Rob Northen compressors. supports all 3 modes.
- Turbo Packer by Wolfgang Mayerle.
- Supports opening XPK-encapsulated files. XPK decompressors supported are MASH, NONE and SQSH

Some decompressors have tested better than others. Support for data verification is severely lacking in most formats.
Also there is not support for password protected files.

Feedback: tz at iki dot fi
