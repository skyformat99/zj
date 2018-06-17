#!/usr/bin/env sh

path=`pwd`/__zjms__
buildpath=`pwd`/__zjmsbuild__
rm -rf $path $buildpath

mkdir $path $buildpath
if [ 0 -ne $? ]; then
    exit 1
fi

cd $buildpath
if [ 0 -ne $? ]; then
    rm -rf $path $buildpath
    exit 1
fi

cmake\
    -DCMAKE_INSTALL_PREFIX=$path\
    -DBUILD_SHARED_LIBS=OFF\
    -DCMAKE_BUILD_TYPE=Release\
    -DBUILD_CLAR=OFF\
    -DTHREADSAFE=OFF\
    ..

if [ 0 -ne $? ]; then
    rm -rf $path $buildpath
    exit 1
fi

njobs=`cat /proc/cpuinfo | grep 'processor' | wc -l`
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
