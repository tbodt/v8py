#!/bin/bash
if [ -d v8 -a ! "$(ls -A v8)" ]; then
    echo 'v8 appears to already have been built'
    exit
fi

# install depot_tools
# honestly, fuck google and their idiot build system
if [ ! -d depot_tools ]; then
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi
export PATH=`pwd`/depot_tools:$PATH

# build v8
fetch v8
cd v8
git checkout branch-heads/5.4
gclient sync
make native -j2
