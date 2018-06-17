#!/usr/bin/env sh

curdir=`pwd`
osname=`uname -s`

echo -e "#ifndef _ZJ_OS_TARGET_H\n#define _ZJ_OS_TARGET_H\n" > ./zj_os_target.h

if [ "FreeBSD" == $osname ]; then
    echo "#define _ZJ_OS_FREEBSD">>./zj_os_target.h
elif [ "Darwin" == $osname ]; then
    echo "#define _ZJ_OS_DARWIN">>./zj_os_target.h
elif [ "Linux" == $osname ]; then
    echo "#define _ZJ_OS_LINUX">>./zj_os_target.h
else
    echo "#define _ZJ_OS_OTHER">>./zj_os_target.h
fi

echo "#define _ZJ_UNIT_TEST_USER \"`whoami`\"">>./zj_os_target.h

echo -e "\n#endif //_ZJ_OS_TARGET_H">>./zj_os_target.h

cd ../deps
./BUILD.sh
cd $curdir
