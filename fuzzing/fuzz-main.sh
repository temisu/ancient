#!/usr/bin/env bash
cd "${0%/*}"
. ./fuzz-settings.sh

# Create tmpfs for storing temporary fuzzing data
mkdir $FUZZING_TEMPDIR
sudo mount -t tmpfs -o size=100M none $FUZZING_TEMPDIR
cp ancient $FUZZING_TEMPDIR/

$AFL_BIN -f $FUZZING_TEMPDIR/infile01 -x all_formats.dict -t $FUZZING_TIMEOUT $FUZZING_INPUT -o $FUZZING_FINDINGS_DIR -M fuzzer01 $FUZZING_TEMPDIR/ancient $FUZZING_TEMPDIR/infile01
