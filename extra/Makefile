# Copyright (C) Teemu Suutari

VPATH  := ../src ../src/common

CC	= clang
CXX	= clang++
PKG_CONFIG = pkg-config
COMMONFLAGS = -g -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-error=multichar -Wno-multichar -I../src `$(PKG_CONFIG) --cflags libbsd`
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14 -fno-rtti
LIBS = `$(PKG_CONFIG) --libs libbsd`
LDFLAGS=$(LIBS)
PROG	= bruteRNC1
OBJS	= Buffer.o SubBuffer.o BruteForceRNC1Encoder.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~ src/*~

.PHONY:
