# Copyright (C) Teemu Suutari

VPATH  := src src/Lzh src/Zip src/common fuzzing

CXX	?= c++
COMMONFLAGS = -Os -Wall -Wsign-compare -Wnarrowing -Wno-error=multichar -Wno-multichar -Isrc -Iapi
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++17 -fno-rtti

PROG	= ancient
MAIN	?= main.o
OBJS	= Buffer.o Common.o MemoryBuffer.o StaticBuffer.o SubBuffer.o CRC16.o CRC32.o \
	Decompressor.o XPKDecompressor.o XPKMain.o \
	OutputStream.o InputStream.o RangeDecoder.o \
	ACCADecompressor.o ARTMDecompressor.o BLZWDecompressor.o BZIP2Decompressor.o \
	CBR0Decompressor.o CRMDecompressor.o CYB2Decoder.o DEFLATEDecompressor.o \
	DLTADecode.o DMSDecompressor.o FASTDecompressor.o FBR2Decompressor.o \
	FRLEDecompressor.o HFMNDecompressor.o HUFFDecompressor.o ILZRDecompressor.o \
	IMPDecompressor.o LHLBDecompressor.o LIN1Decompressor.o LIN2Decompressor.o \
	LZBSDecompressor.o LZCBDecompressor.o LZW2Decompressor.o LZW4Decompressor.o \
	LZW5Decompressor.o LZXDecompressor.o MASHDecompressor.o MMCMPDecompressor.o \
	NONEDecompressor.o NUKEDecompressor.o PPDecompressor.o RAKEDecompressor.o \
	RDCNDecompressor.o RLENDecompressor.o RNCDecompressor.o SDHCDecompressor.o \
	SHR3Decompressor.o SHRIDecompressor.o SLZ3Decompressor.o SMPLDecompressor.o \
	StoneCrackerDecompressor.o SQSHDecompressor.o SXSCDecompressor.o TDCSDecompressor.o \
	TPWMDecompressor.o  ZENODecompressor.o LH1Decompressor.o LH2Decompressor.o \
	LH3Decompressor.o LHXDecompressor.o LZ5Decompressor.o LZSDecompressor.o \
	PMDecompressor.o LZHDecompressor.o ImplodeDecompressor.o ReduceDecompressor.o \
	ShrinkDecompressor.o ZIPDecompressor.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(PROG): $(OBJS) $(MAIN)
	$(CXX) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) $(MAIN) $(PROG) *~ src/*~

.PHONY:
