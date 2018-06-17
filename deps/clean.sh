#!/usr/bin/env sh

curpath=`pwd`

echo -e "\x1b[31;01mrm -rf:\x1b[00m"

idx=0
first=""
for dep in `find . -maxdepth 1 -type d`; do
    if [ '.' == $dep ]; then continue; fi
    if [ '..' == $dep ]; then continue; fi

    rm -rf ${dep}/__zjms__

    if [ 0 -eq $idx ]; then
        first=$dep/__zjms__
    else
        echo -e "    ├── $dep/__zjms__"
    fi

    let idx++
done

if [ "" != $first ]; then
    echo -e "    └── $first"
fi
