#!/usr/bin/env sh

rm -rf __zjmsbuild__ 2>/dev/null
mkdir __zjmsbuild__
cd __zjmsbuild__

cmake -G "Unix Makefiles" .. \
	-DCMAKE_INSTALL_PREFIX=`dirname \`pwd\``/__zjms__ \
	-DBUILD_SHARED_LIBS=OFF \
	-DNNG_TESTS=OFF \
	-DNNG_TOOLS=OFF \
	-DNNG_ENABLE_TLS=OFF \
	-DNNG_TRANSPORT_ZEROTIER=OFF \
	-DNNG_ENABLE_HTTP=OFF \
    -DNNG_TRANSPORT_WS=OFF \
    -DCMAKE_BUILD_TYPE=Release

make
make install/strip
make clean
