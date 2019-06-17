# Copyright (C) Teemu Suutari

VPATH  := src src/Lzh src/Zip src/common

CC	= clang
CXX	= clang++
PKG_CONFIG = pkg-config
COMMONFLAGS = -Os -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-error=multichar -Wno-multichar -Isrc `$(PKG_CONFIG) --cflags libbsd`
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14 -fno-rtti
LIBS = `$(PKG_CONFIG) --libs libbsd`
LDFLAGS = $(LIBS)

PROG	= ancient
OBJS	= Buffer.o Common.o MemoryBuffer.o StaticBuffer.o SubBuffer.o CRC16.o CRC32.o \
	Decompressor.o XPKDecompressor.o XPKMaster.o \
	OutputStream.o InputStream.o \
	ACCADecompressor.o BLZWDecompressor.o BZIP2Decompressor.o CBR0Decompressor.o \
	CRMDecompressor.o CYB2Decoder.o DEFLATEDecompressor.o DLTADecode.o \
	DMSDecompressor.o FASTDecompressor.o FBR2Decompressor.o FRLEDecompressor.o \
	HFMNDecompressor.o HUFFDecompressor.o ILZRDecompressor.o IMPDecompressor.o \
	LHLBDecompressor.o LIN1Decompressor.o LIN2Decompressor.o LZBSDecompressor.o \
	LZW2Decompressor.o LZW4Decompressor.o LZW5Decompressor.o LZXDecompressor.o \
	MASHDecompressor.o NONEDecompressor.o NUKEDecompressor.o PPDecompressor.o \
	RAKEDecompressor.o RDCNDecompressor.o RLENDecompressor.o RNCDecompressor.o \
	SDHCDecompressor.o SHR3Decompressor.o SHRIDecompressor.o SLZ3Decompressor.o \
	SMPLDecompressor.o SQSHDecompressor.o SXSCDecompressor.o TDCSDecompressor.o \
	TPWMDecompressor.o ZENODecompressor.o \
	LH1Decompressor.o LH2Decompressor.o LH3Decompressor.o LHXDecompressor.o \
	LZ5Decompressor.o LZSDecompressor.o PMDecompressor.o LZHDecompressor.o \
	ImplodeDecompressor.o ReduceDecompressor.o ShrinkDecompressor.o ZIPDecompressor.o \
	main.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROG): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~ src/*~

.PHONY:
