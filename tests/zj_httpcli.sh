#!/usr/bin/env sh

(go run http_server.go) &

sleep 2

name=zj_httpcli
path=/tmp/__zj__

mkdir -p $path
if [ 0 -ne $? ]; then
    echo "invalid: $path"
    exit 1
fi

cc ../src/${name}.c ../deps/nng-1.0.0/__zj__/lib/libnng.a\
    -D_ZJ_UNIT_TEST\
    -g -lpthread\
	-I../deps/nng-1.0.0/__zj__/include\
	-I../src\
    -I./c-convey\
    -o ${path}/${name}

if [ 0 -ne $? ]; then
    echo "cc failed"
    exit 1
fi

${path}/${name}
