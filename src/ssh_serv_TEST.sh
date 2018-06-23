#!/usr/bin/env sh

path=/tmp/__zjms__

mkdir -p $path
if [ 0 -ne $? ]; then
	echo "invalid: $path"
	exit 1
fi

cc -Wall -Wextra -g -lssh -lssh_threads -D_UNIT_TEST -I../tests/c-convey ssh_serv.c threadpool.c -o $path/ssh_serv_TEST
if [ 0 -ne $? ]; then
	echo "cc failed"
	exit 1
fi

$path/ssh_serv_TEST
