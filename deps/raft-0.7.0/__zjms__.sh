#!/usr/bin/env sh

path=`pwd`/__zjms__
rm -rf $path

mkdir -p $path/lib
if [ 0 -ne $? ]; then
    exit 1
fi

make static
mv libraft.a $path/lib/
cp -r include $path/
make clean
