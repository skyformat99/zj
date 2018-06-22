#!/usr/bin/env sh

path=`pwd`/__zjms__
rm -rf $path 2>/dev/null

cc -O2 -c -o threadpool.o threadpool.c
ar -crv threadpool.a threadpool.o
rm threadpool.o

mkdir -p ${path}/include
mkdir ${path}/lib

cp threadpool.h ${path}/include/
mv threadpool.a ${path}/lib/
