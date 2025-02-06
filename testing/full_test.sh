#!/bin/bash

export BIN=./obj/ancient
export TEST_V=./test_vectors


for a in $TEST_V/compact_files/*.C ; do $BIN verify $a $(echo $a | sed s/\\.C/\\.raw/) || exit 1 ; done

for a in $TEST_V/compress_files/*.Z ; do $BIN verify $a $(echo $a | sed s/\\.Z/\\.raw/) || exit 1 ; done
for a in $TEST_V/compress_files2/*.Z ; do $BIN verify $a $(echo $a | sed s/\\.Z/\\.raw/) || exit 1 ; done

for a in $TEST_V/dms_files/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files2/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files3/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files4/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done
for a in $TEST_V/dms_files5/*.dms ; do $BIN verify $a $(echo $a | sed s/\\.dms/\\.adf/) || exit 1 ; done

for a in $TEST_V/edam_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/freeze_files/*.F ; do $BIN verify $a $(echo $a | sed s/\\.F/\\.raw/) || exit 1 ; done
for a in $TEST_V/freeze_files2/*.F ; do $BIN verify $a $(echo $a | sed s/\\.F/\\.raw/) || exit 1 ; done
for a in $TEST_V/freeze_files3/*.F ; do $BIN verify $a $(echo $a | sed s/\\.F/\\.raw/) || exit 1 ; done

for a in $TEST_V/good_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/gzbz2_files/*.gz ; do $BIN verify $a $(echo $a | sed s/\\.gz/\\.raw/) || exit 1 ; done
for a in $TEST_V/gzbz2_files/*.bz2 ; do $BIN verify $a $(echo $a | sed s/\\.bz2/\\.raw/) || exit 1 ; done

for a in $TEST_V/lob_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/lob_files2/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
# 1 extra byte
for a in $TEST_V/lob_files3/*.lob ; do $BIN verify $a $(echo $a | sed s/\\_m.\*.lob/\\.raw/) ; done

for a in $TEST_V/mh_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/mmcmp_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) ; done

for a in $TEST_V/pack_files/*.z ; do $BIN verify $a $(echo $a | sed s/\\.z/\\.raw/) || exit 1 ; done
for a in $TEST_V/pack_files2/*.z ; do $BIN verify $a $(echo $a | sed s/\\.z/\\.raw/) || exit 1 ; done

for a in $TEST_V/ice_files/*.ice ; do $BIN verify $a $(echo $a | sed s/\\.ice/\\.raw/) || exit 1 ; done
for a in $TEST_V/tmm_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/tsm_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/tsm_files2/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/she_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/pmc_files/*.pmc ; do $BIN verify $a $(echo $a | sed s/\\.pmc/\\.raw/) || exit 1 ; done

for a in $TEST_V/pp_files/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files2/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files3/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files4/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done
for a in $TEST_V/pp_files5/*.pp ; do $BIN verify $a $(echo $a | sed s/\\.pp/\\.raw/) || exit 1 ; done

for a in $TEST_V/quasijarus_files/*.Z ; do $BIN verify $a $(echo $a | sed s/\\.Z/\\.raw/) || exit 1 ; done

for a in $TEST_V/rdc9_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/regression_test/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/rnc1pc_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/rnc2_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/sco_files/*.Z ; do $BIN verify $a $(echo $a | sed s/\\.Z/\\.raw/) || exit 1 ; done

# some decompressors generate 1 bytes extra. manual check needed for results
for a in $TEST_V/stonecracker_files/*.pack* ; do $BIN verify $a $(echo $a | sed s/\\.pack.*/\\.raw/) ; done

for a in $TEST_V/vice_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/vic2_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/xpk_testfiles/*.pack ; do $BIN verify $a $(echo $a | sed s/\\_xpk.*.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/xpk_testfiles2/*.pack ; do $BIN verify $a $(echo $a | sed s/\\_xpk.*.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/xpk_testfiles3/*.xpk ; do $BIN verify $a $(echo $a | sed s/\\.xpk/\\.raw/) || exit 1 ; done

for a in $TEST_V/xtra_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/am_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/bifi_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/crossroad_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/fc_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/fears_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/hanxiety_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/hoi_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/hopp_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/mss_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/rebels_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/rebels_files2/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/shining_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/skyhigh_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/sun_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/tcarnage_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done

for a in $TEST_V/ace_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/grac_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
for a in $TEST_V/md_files/*.pack ; do $BIN verify $a $(echo $a | sed s/\\.pack/\\.raw/) || exit 1 ; done
