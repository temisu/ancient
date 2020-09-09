#!/usr/bin/env bash
cd "${0%/*}"
cd ..
CXX=fuzzing/afl/afl-clang-fast++ LD=fuzzing/afl/afl-clang-fast++ MAIN=fuzz.o AFL_HARDEN=1 make clean all
