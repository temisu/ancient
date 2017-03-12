# Copyright (C) Teemu Suutari

CC	= clang
CXX	= clang++
COMMONFLAGS = -Os -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-error=multichar -Wno-multichar
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14

PROG	= decompress
OBJS	= Buffer.o CRC32.o CRMDecompressor.o Decompressor.o \
	DEFLATEDecompressor.o IMPDecompressor.o \
	MASHDecompressor.o NONEDecompressor.o RNCDecompressor.o \
	SQSHDecompressor.o TPWMDecompressor.o XPKMaster.o main.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~

.PHONY:
