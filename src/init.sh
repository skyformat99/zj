#!/usr/bin/env bash

curdir=`pwd`
cd ../deps
./BUILD.sh
cd $curdir

osname=`uname -s`

echo "#ifndef OS_TARGET_H" > ./os_target.h
echo "#define OS_TARGET_H" >> ./os_target.h

echo "#define Z_RELEASE" >> ./os_target.h

if [[ "FreeBSD" == $osname ]]; then
    echo "#define OS_FREEBSD">>./os_target.h
elif [[ "Darwin" == $osname ]]; then
    echo "#define OS_DARWIN">>./os_target.h
elif [[ "Linux" == $osname ]]; then
    echo "#define OS_LINUX">>./os_target.h
else
    echo "#define OS_OTHER">>./os_target.h
fi

echo "#define UNIT_TEST_USER \"`whoami`\"">>./os_target.h

echo "#endif //OS_TARGET_H">>./os_target.h
