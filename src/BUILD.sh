#!/usr/bin/env sh

osname=`uname -s`

if [ "FreeBSD" == $osname ]; then
	echo "#define _OS_FREEBSD_ZJ" > ./zj_os_target.h
elif [ "Darwin" == $osname ]; then
	echo "#define _OS_DARWIN_ZJ" > ./zj_os_target.h
elif [ "Linux" == $osname ]; then
	echo "#define _OS_LINUX_ZJ" > ./zj_os_target.h
else
	echo "#define _OS_OTHER_ZJ" > ./zj_os_target.h
fi

echo "#define __UNIT_TEST_USERNAME \"`whoami`\"" >> ./zj_os_target.h

# TODO
