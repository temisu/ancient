# Copyright (C) Teemu Suutari

VPATH  := ../src

CC	= clang
CXX	= clang++
COMMONFLAGS = -g -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-error=multichar -Wno-multichar -I../src
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14 -fno-rtti -fno-exceptions

PROG	= bruteRNC1
OBJS	= Buffer.o SubBuffer.o BruteForceRNC1Encoder.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~ src/*~

.PHONY:
