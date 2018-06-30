#!/usr/bin/env bash

if [[ 0 -eq `grep "\`cat ~/.ssh/id_rsa.pub\`" ~/.ssh/authorized_keys | wc -l` ]]; then
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
    chmod 0600 ~/.ssh/authorized_keys
fi

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
    -lssh -lssh_threads -lpthread

if [[ 0 -ne $? ]]; then
    echo "cc failed"
    exit 1
fi

${path}/${name}
