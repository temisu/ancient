#!/usr/bin/env bash
cd "${0%/*}"

AFL_VERSION="$(wget --quiet -O - "https://api.github.com/repos/AFLplusplus/AFLplusplus/releases/latest" | jq -r .tag_name)"
AFL_FILENAME="$AFL_VERSION.tar.gz"
AFL_URL="https://github.com/AFLplusplus/AFLplusplus/archive/refs/tags/$AFL_FILENAME"

rm $AFL_FILENAME
wget $AFL_URL || exit
tar -xzvf $AFL_FILENAME
rm $AFL_FILENAME
cd AFLplusplus-*
make source-only || exit
cd ..
rm -rf afl
mv AFLplusplus-* afl
