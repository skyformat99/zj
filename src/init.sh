#!/usr/bin/env bash

curdir=`pwd`
cd ../deps
./BUILD.sh
cd $curdir

osname=`uname -s`

echo "#ifndef _ENV_H" > ./env.h
echo "#define _ENV_H" >> ./env.h

if [[ "FreeBSD" == $osname ]]; then
    echo "#define _OS_FREEBSD">>./env.h
elif [[ "Linux" == $osname ]]; then
    echo "#define _OS_LINUX">>./env.h
else
    echo "#define _OS_OTHER">>./env.h
fi

echo "#define _UNIT_TEST_USER \"`whoami`\"" >>./env.h

echo "#endif //_ENV_H">>./env.h
