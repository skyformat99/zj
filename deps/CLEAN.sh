#!/usr/bin/env bash

root=".."
mark=`cat ${root}/mark`
curpath=`pwd`

echo "rm -rf:"

idx=0
first=""
for dep in `find . -maxdepth 1 -type d`; do
    if [[ '.' == $dep ]]; then continue; fi
    if [[ '..' == $dep ]]; then continue; fi

    rm -rf ${dep}/${mark}

    if [[ 0 -eq $idx ]]; then
        first=$dep/${mark}
    else
        echo "    ├── $dep/${mark}"
    fi

    idx=1
done

if [[ "" != $first ]]; then
    echo "    └── $first"
fi
