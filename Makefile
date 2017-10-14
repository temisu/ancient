# Copyright (C) Teemu Suutari

VPATH  := src

CC	= clang
CXX	= clang++
COMMONFLAGS = -Os -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-error=multichar -Wno-multichar -Isrc
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14 -fno-rtti

PROG	= ancient
OBJS	= Buffer.o SubBuffer.o CRC32.o \
	Decompressor.o XPKDecompressor.o XPKMaster.o main.o \
	ACCADecompressor.o BLZWDecompressor.o BZIP2Decompressor.o CBR0Decompressor.o \
	CRMDecompressor.o CYB2Decoder.o DEFLATEDecompressor.o DLTADecode.o \
	FASTDecompressor.o FBR2Decompressor.o FRLEDecompressor.o HFMNDecompressor.o \
	HUFFDecompressor.o ILZRDecompressor.o IMPDecompressor.o LHLBDecompressor.o \
	LIN1Decompressor.o LIN2Decompressor.o LZBSDecompressor.o LZW2Decompressor.o \
	LZW4Decompressor.o LZW5Decompressor.o LZXDecompressor.o MASHDecompressor.o \
	NONEDecompressor.o NUKEDecompressor.o PPDecompressor.o RAKEDecompressor.o \
	RDCNDecompressor.o RLENDecompressor.o RNCDecompressor.o SDHCDecompressor.o \
	SHR3Decompressor.o SHRIDecompressor.o SLZ3Decompressor.o SMPLDecompressor.o \
	SQSHDecompressor.o TDCSDecompressor.o TPWMDecompressor.o ZENODecompressor.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~ src/*~

test: $(PROG)
	for a in gzbz2_files/*.gz ; do ./ancient verify $$a $$(echo $$a | sed s/\\.gz/.raw/) >/dev/null ; done
	for a in gzbz2_files/*.bz2 ; do ./ancient verify $$a $$(echo $$a | sed s/\\.bz2/.raw/) >/dev/null ; done
	for a in xpk_testfiles/*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/_xpk.*/.raw/) >/dev/null ; done
	for a in pp_files/test*.pp ; do ./ancient verify $$a $$(echo $$a | sed s/\\.pp/.raw/) >/dev/null ; done
	for a in xtra_files/test*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/pack/raw/) >/dev/null ; done
	for a in good_files/test*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/pack/raw/) >/dev/null ; done
	for a in regression_test/test*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/pack/raw/) >/dev/null ; done

.PHONY:
