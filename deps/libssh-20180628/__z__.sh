#!/usr/bin/env bash

if [[ 0 -eq $# ]]; then
    echo "param invalid!"
    exit 1
fi

path=`pwd`/${1}
buildpath=`pwd`/${1}build
rm -rf $path $buildpath 2>/dev/null

mkdir $buildpath
cd $buildpath
if [[ 0 -ne $? ]]; then
    exit 1
fi

if [[ "Darwin" == `uname -s` ]]; then
    export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
fi

# WITH_NACL：依赖 libnacl，提供curve25519加密算法(Curve25519是一个椭圆曲线,提供128位的安全性,并设计用于椭圆曲线Diffie-Hellman(ECDH)密钥协商方案。它是最快的ECC曲线之一,并没有被任何已知的专利所涵盖)。
# WITH_PACP：网络调试、监控用途
cmake\
    -DCMAKE_INSTALL_PREFIX=$path\
    -DCMAKE_BUILD_TYPE=Release\
    \
    -DWITH_SERVER=OFF\
    -DWITH_STATIC_LIB=OFF\
    -DWITH_ZLIB=ON\
    -DWITH_NACL=ON\
    \
    -DWITH_SSH1=OFF\
    -DWITH_SFTP=OFF\
    -DWITH_GSSAPI=OFF\
    -DWITH_DEBUG_CRYPTO=OFF\
    -DWITH_DEBUG_CALLTRACE=OFF\
    -DWITH_PCAP=OFF\
    -DWITH_GCRYPT=OFF\
    \
    -DWITH_EXAMPLES=OFF\
    -DWITH_BENCHMARKS=OFF\
    -DWITH_CLIENT_TESTING=OFF\
    -DWITH_TESTING=OFF\
    -DWITH_INTERNAL_DOC=OFF\
    ..

if [[ 0 -ne $? ]]; then
    rm -rf $buildpath
    exit 1
fi

njobs=`cat /proc/cpuinfo 2>/dev/null | grep 'processor' | wc -l`
if [[ 0 -eq $njobs ]]; then
    njobs=2
fi

make -j $njobs
if [[ 0 -ne $? ]]; then
    rm -rf $buildpath
    exit 1
fi

make install
make clean
rm -rf $buildpath
