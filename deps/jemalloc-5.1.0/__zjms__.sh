#!/usr/bin/env sh

path=`pwd`/__zjms__
rm -rf $path

./autogen.sh
if [ 0 -ne $? ]; then
    exit 1
fi

./configure --prefix=$path --with-jemalloc-prefix=j --without-export --disable-cxx
if [ 0 -ne $? ]; then
    exit 1
fi

njobs=`cat /proc/cpuinfo | grep 'processor' | wc -l`
if [ 0 -eq $njobs ]; then
    njobs=2
fi

make -j $njobs install_lib_static
if [ 0 -ne $? ]; then
    rm -rf $path
    exit 1
fi

make -j $njobs install_include
if [ 0 -ne $? ]; then
    rm -rf $path
    exit 1
fi

make distclean
