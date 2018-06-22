#!/usr/bin/env sh

path=`pwd`/__zjms__
buildpath=`pwd`/__zjmsbuild__
rm -rf $path $buildpath 2>/dev/null

mkdir __zjmsbuild__
if [ 0 -ne $? ]; then
	exit 1
fi

cd __zjmsbuild__
if [ 0 -ne $? ]; then
	exit 1
fi

cmake -G "Unix Makefiles" .. \
	-DCMAKE_INSTALL_PREFIX=$path \
	-DBUILD_SHARED_LIBS=OFF \
	-DNNG_TESTS=OFF \
	-DNNG_TOOLS=OFF \
	-DNNG_ENABLE_TLS=OFF \
	-DNNG_TRANSPORT_ZEROTIER=OFF \
	-DNNG_ENABLE_HTTP=OFF \
    -DNNG_TRANSPORT_WS=OFF \
    -DCMAKE_BUILD_TYPE=Release

if [ 0 -ne $? ]; then
	rm -rf $path $buildpath
	exit 1
fi

make
if [ 0 -ne $? ]; then
	rm -rf $path $buildpath
	exit 1
fi

make install/strip
if [ 0 -ne $? ]; then
	rm -rf $path $buildpath
	exit 1
fi

make clean
rm -rf $buildpath
