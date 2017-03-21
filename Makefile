# Copyright (C) Teemu Suutari

CC	= clang
CXX	= clang++
COMMONFLAGS = -Os -Wall -Wsign-compare -Wshorten-64-to-32 -Wno-error=multichar -Wno-multichar -I.
CFLAGS	= $(COMMONFLAGS)
CXXFLAGS = $(COMMONFLAGS) -std=c++14

PROG	= ancient
OBJS	= Buffer.o CRC32.o CBR0Decompressor.o CRMDecompressor.o Decompressor.o \
	DEFLATEDecompressor.o DLTADecode.o FASTDecompressor.o FRLEDecompressor.o \
	HFMNDecompressor.o HUFFDecompressor.o IMPDecompressor.o MASHDecompressor.o NONEDecompressor.o \
	NUKEDecompressor.o PPDecompressor.o RLENDecompressor.o RNCDecompressor.o \
	SQSHDecompressor.o TPWMDecompressor.o XPKDecompressor.o XPKMaster.o main.o

all: $(PROG)

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROG): $(OBJS)
	$(CXX) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -f $(OBJS) $(PROG) *~

test: $(PROG)
	for a in xpk_testfiles/*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/_xpk.*/.raw/) >/dev/null ; done
	for a in pp_files/test*.pp ; do ./ancient verify $$a $$(echo $$a | sed s/\\.pp/.raw/) >/dev/null ; done
	for a in xtra_files/test*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/pack/raw/) >/dev/null ; done
	for a in good_files/test*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/pack/raw/) >/dev/null ; done
	for a in regression_test/test*.pack ; do ./ancient verify $$a $$(echo $$a | sed s/pack/raw/) >/dev/null ; done

.PHONY:
