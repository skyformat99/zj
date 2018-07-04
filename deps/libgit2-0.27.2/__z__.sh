#!/usr/bin/env bash

if [[ 0 -eq $# ]]; then
    echo "param invalid!"
    exit 1
fi

path=`pwd`/${1}
buildpath=`pwd`/${1}build
rm -rf $path $buildpath

mkdir $buildpath
if [[ 0 -ne $? ]]; then
    exit 1
fi

cd $buildpath
if [[ 0 -ne $? ]]; then
    rm -rf $path $buildpath
    exit 1
fi

cmake\
    -DCMAKE_INSTALL_PREFIX=$path\
    -DBUILD_SHARED_LIBS=ON\
    -DCMAKE_BUILD_TYPE=Release\
    -DBUILD_CLAR=OFF\
    -DTHREADSAFE=ON\
    ..

if [[ 0 -ne $? ]]; then
    rm -rf $path $buildpath
    exit 1
fi

njobs=`cat /proc/cpuinfo 2>/dev/null | grep 'processor' | wc -l`
if [[ 0 -eq $njobs ]]; then
    njobs=2
fi

make -j $njobs
if [[ 0 -ne $? ]]; then
    rm -rf $path $buildpath
    exit 1
fi

make install
if [[ 0 -ne $? ]]; then
    rm -rf $path $buildpath
    exit 1
fi

make clean
rm -rf $buildpath
