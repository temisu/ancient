#!/usr/bin/env bash

# Input data for fuzzer
# If you run the fuzzer for the first time, specify a directory with some input
# files for the fuzzer, e.g.
#     FUZZING_INPUT="-i /home/foo/testcases/"
# If you want to continue fuzzing using the previous findings, use:
#     FUZZING_INPUT=-i-
FUZZING_INPUT="-i /path/to/initial"

# Directory to place temporary fuzzing data into
FUZZING_TEMPDIR=~/fuzzers/ancient-temp
# Directory to store permanent fuzzing data (e.g. found crashes) into
FUZZING_FINDINGS_DIR=~/fuzzers/ancient-fuzzing
# Fuzzer timeout in ms, + = don't abort on timeout
FUZZING_TIMEOUT=5000+
# Path to afl-fuzz binary
AFL_BIN=afl/afl-fuzz