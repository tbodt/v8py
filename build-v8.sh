#!/bin/bash

function run() {
    echo "$ $@"
    $@
    if [ $? -ne 0 ]; then
        exit
    fi
}

if [ -d v8/.git ]; then
    echo 'v8 appears to already have been built'
    exit
else
    rm -rf v8
fi

# install depot_tools
# honestly, fuck google and their idiot build system
if [ ! -d depot_tools ]; then
    run git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi
export PATH=`pwd`/depot_tools:$PATH

# build v8
run fetch v8
run cd v8
run git checkout branch-heads/5.4
run gclient sync
# v8 uses clang warning flags that are too bleeding edge
# this would be ok, but they also use -Werror
run sed -i.bak /no-undefined-var-template/d gypfiles/standalone.gypi
run sed -i.bak /no-nonportable-include-path/d gypfiles/standalone.gypi
run make native -j2
