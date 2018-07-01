#!/usr/bin/env bash

curdir=`pwd`
cd ../deps
./BUILD.sh
cd $curdir

osname=`uname -s`

echo "#ifndef _OS_TARGET_H" > ./env.h
echo "#define _OS_TARGET_H" >> ./env.h

if [[ "FreeBSD" == $osname ]]; then
    echo "#define _OS_FREEBSD">>./env.h
elif [[ "Darwin" == $osname ]]; then
    echo "#define _OS_DARWIN">>./env.h
elif [[ "Linux" == $osname ]]; then
    echo "#define _OS_LINUX">>./env.h
else
    echo "#define _OS_OTHER">>./env.h
fi

echo "#define UNIT_TEST_USER \"`whoami`\"">>./env.h

echo "#endif //_OS_TARGET_H">>./env.h
