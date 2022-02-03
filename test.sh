#!/bin/bash

export BIN=./ancient 
export TEST_V=../../system/test_vectors

for a in $TEST_V/dms_files/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files2/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files3/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files4/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done

for a in $TEST_V/edam_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/good_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/gzbz2_files/*.gz ; do $BIN verify $a $(echo $a | sed s/\\.gz/\\.raw/) || exit 1 ; done
for a in $TEST_V/gzbz2_files/*.bz2 ; do $BIN verify $a $(echo $a | sed s/\\.bz2/\\.raw/) || exit 1 ; done


for a in $TEST_V/mh_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/lob_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/lob_files2/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files2/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files3/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files4/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files5/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/rdc9_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/regression_test/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/rnc1pc_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

# some decompressors generate 1 bytes extra. manual check needed for results
for a in $TEST_V/stonecracker_files/*.pack* ; do $BIN verify $a $(echo $a | sed s/\\.pack.*/\\.raw/) ; done

for a in $TEST_V/xpk_testfiles/*.pack ; do test $(echo $a | sed s/.*_xpkppmq.*//) && $BIN verify $a $(echo $a | sed s/\\_xpk.*.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/xpk_testfiles2/*.pack ; do $BIN verify $a $(echo $a | sed s/\\_xpk.*.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/xtra_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done