#!/usr/bin/env bash
cd "${0%/*}"
cd ..
CXX=fuzzing/afl/afl-clang-fast++ LD=fuzzing/afl/afl-clang-fast++ MAIN_N=fuzz.o AFL_HARDEN=1 make -f Makefile.unix clean all
