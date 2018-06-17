#!/usr/bin/env sh

curpath=`pwd`

for dep in `find . -maxdepth 1 -type d`; do
    if [ '.' == $dep ]; then continue; fi
    if [ '..' == $dep ]; then continue; fi

    if [ 0 -eq `ls -d ${dep}/__zj__ 2>/dev/null | wc -l` ]; then
        cd $dep
        echo "===> enter: $dep"
        ./__zj__.sh >/dev/null
    fi

    cd $curpath
done
