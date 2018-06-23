#!/usr/bin/env sh

path=/tmp/__zjms__

mkdir -p $path
if [ 0 -ne $? ]; then
    echo "invalid: $path"
    exit 1
fi

cc ../src/ssh_serv.c ../src/threadpool.c\
    -D_UNIT_TEST\
    -g -lssh -lssh_threads -lutil\
	-L../deps/libssh-0.7.5/__zjms__/lib\
	-I../deps/libssh-0.7.5/__zjms__/include\
	-I../src\
    -I./c-convey\
    -o $path/ssh_serv

if [ 0 -ne $? ]; then
    echo "cc failed"
    exit 1
fi

export LD_LIBRARY_PATH="../deps/libssh-0.7.5/__zjms__/lib"
$path/ssh_serv
