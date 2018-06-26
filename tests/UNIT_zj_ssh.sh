#!/usr/bin/env sh

name=zj_ssh
path=/tmp/__zj__

mkdir -p $path
if [ 0 -ne $? ]; then
    echo "invalid: $path"
    exit 1
fi

cc ../src/${name}.c\
    -D_ZJ_UNIT_TEST\
    -g -lssh -lssh_threads -lpthread\
	-L../deps/libssh-0.7.5/__zj__/lib\
	-I../deps/libssh-0.7.5/__zj__/include\
	-I../src\
    -I./c-convey\
    -o ${path}/${name}

if [ 0 -ne $? ]; then
    echo "cc failed"
    exit 1
fi

export LD_LIBRARY_PATH="../deps/libssh-0.7.5/__zj__/lib"
${path}/${name}