#!/usr/bin/env sh

path=/tmp/__zj__

mkdir -p $path
if [ 0 -ne $? ]; then
    echo "invalid: $path"
    exit 1
fi

cc ../src/zj_ssh.c\
    -D_ZJ_UNIT_TEST\
    -g -lssh -lssh_threads -lutil -lpthread\
	-L../deps/libssh-0.7.5/__zj__/lib\
	-I../deps/libssh-0.7.5/__zj__/include\
	-I../src\
    -I./c-convey\
    -o $path/zj_ssh

if [ 0 -ne $? ]; then
    echo "cc failed"
    exit 1
fi

export LD_LIBRARY_PATH="../deps/libssh-0.7.5/__zj__/lib"
$path/zj_ssh
