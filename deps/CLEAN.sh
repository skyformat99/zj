#!/usr/bin/env sh

curpath=`pwd`

echo "rm -rf:"

idx=0
first=""
for dep in `find . -maxdepth 1 -type d`; do
    if [ '.' == $dep ]; then continue; fi
    if [ '..' == $dep ]; then continue; fi

    rm -rf ${dep}/__zj__

    if [ 0 -eq $idx ]; then
        first=$dep/__zj__
    else
        echo "    ├── $dep/__zj__"
    fi

    idx=1
done

if [ "" != $first ]; then
    echo "    └── $first"
fi
