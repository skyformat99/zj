#!/usr/bin/env sh

./autogen.sh
if [ 0 -ne $? ]; then
	exit 1
fi

./configure --prefix=`pwd`/__zjms__ --with-jemalloc-prefix=j --without-export --disable-cxx
if [ 0 -ne $? ]; then
	exit 1
fi

make install_include
if [ 0 -ne $? ]; then
	exit 1
fi

make install_lib_static
if [ 0 -ne $? ]; then
	exit 1
fi

make distclean
