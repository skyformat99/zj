#!/usr/bin/env sh

cc -O2 -c -o threadpool.o threadpool.c
ar -crv threadpool.a threadpool.o
rm threadpool.o

mkdir __zjms__
mkdir __zjms__/include
mkdir __zjms__/lib

cp threadpool.h __zjms__/include/
mv threadpool.a __zjms__/lib/
