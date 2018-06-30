#!/usr/bin/env bash

mark=`cat ${ZJMS_PATH}/mark`
name=`basename $0 | grep -o '^[^.]\+'`
path=/tmp/${mark}

mkdir -p $path
if [[ 0 -ne $? ]]; then
    echo "invalid: $path"
    exit 1
fi

# **** #
eval cc -g ${name}.c `cat ${ZJMS_PATH}/ccpath` -o ${path}/${name}\
    "`perl -ane 'print "$F[1]" if /^-L.*nng/' ${ZJMS_PATH}/ccpath | head -1`/libnng.a"\
    -lpthread

if [[ 0 -ne $? ]]; then
    echo "cc failed"
    exit 1
fi

${path}/${name}
