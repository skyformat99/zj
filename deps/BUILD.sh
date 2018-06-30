#!/usr/bin/env bash

curpath=`pwd`
root=".."
mark=`cat ${root}/mark`

for dep in `find . -maxdepth 1 -type d`; do
    if [[ '.' == $dep ]]; then continue; fi
    if [[ '..' == $dep ]]; then continue; fi

    if [[ 0 -eq `ls -d ${dep}/${mark} 2>/dev/null | wc -l` ]]; then
        cd $dep
        echo "===> enter: $dep"
        ./${mark}.sh ${mark} >/dev/null
    fi

    cd $curpath
done
