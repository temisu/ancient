# Copyright (C) Teemu Suutari

CC	= clang
CXX	= clang++
COMMONFLAGS = -Os -Wall -Werror -Wsign-compare -Wshorten-64-to-32
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14

PROG	= decompress
OBJS	= Buffer.o CRMDecompressor.o Decompressor.o IMPDecompressor.o \
	MASHDecompressor.o NONEDecompressor.o RNCDecompressor.o \
	SQSHDecompressor.o TPWMDecompressor.o XPKMaster.o main.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) -c $<

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~

.PHONY:
