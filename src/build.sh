#!/usr/bin/env bash

name=`zj`
mark=`cat ${ZJ_PATH}/mark`
path=$HOME

source_files=`find ${ZJ_PATH}/src -type f -name '*.c' | grep -v '_test.c'`

# **** #
eval cc ${source_files}\
    -O2\
    -D_RELEASE\
    "`perl -ane 'print "$F[1]" if /^-L.*nng/' ${ZJ_PATH}/ccpath | head -1`/libnng.a"\
    `cat ${ZJ_PATH}/ccpath`\
    -lpthread\
    -o ${path}/${name}

if [[ 0 -ne $? ]]; then
    echo "cc failed"
    exit 1
fi
