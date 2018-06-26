#!/usr/bin/env sh

path=`pwd`/__zj__
buildpath=`pwd`/__zjbuild__
rm -rf $path $buildpath 2>/dev/null

mkdir __zjbuild__
if [ 0 -ne $? ]; then
    exit 1
fi

cd __zjbuild__
if [ 0 -ne $? ]; then
    exit 1
fi

cmake -G "Unix Makefiles" ..\
    -DCMAKE_INSTALL_PREFIX=$path\
    -DBUILD_SHARED_LIBS=OFF\
    -DNNG_TRANSPORT_INPROC=OFF\
    -DNNG_TESTS=OFF\
    -DNNG_TOOLS=OFF\
    -DNNG_ENABLE_TLS=OFF\
    -DNNG_TRANSPORT_ZEROTIER=OFF\
    -DNNG_ENABLE_HTTP=ON\
    -DNNG_TRANSPORT_WS=OFF\
    -DCMAKE_BUILD_TYPE=Release

if [ 0 -ne $? ]; then
    rm -rf $path $buildpath
    exit 1
fi

njobs=`cat /proc/cpuinfo 2>/dev/null | grep 'processor' | wc -l`
if [ 0 -eq $njobs ]; then
    njobs=2
fi

make -j $njobs
if [ 0 -ne $? ]; then
    rm -rf $path $buildpath
    exit 1
fi

make install
if [ 0 -ne $? ]; then
    rm -rf $path $buildpath
    exit 1
fi

make clean
rm -rf $buildpath
