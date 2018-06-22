$!/usr/bin/env sh

osname=`uname -s`

if [ "FreeBSD" == $osname ]; then
	echo "#define _OS_FREEBSD" > ./os_target.h
elif [ "Darwin" == $osname ]; then
	echo "#define _OS_DARWIN" > ./os_target.h
elif [ "Linux" == $osname ]; then
	echo "#define _OS_LINUX" > ./os_target.h
else
	echo "#define _OS_OTHER" > ./os_target.h
fi
