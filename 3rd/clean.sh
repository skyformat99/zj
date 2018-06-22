#!/usr/bin/env sh

curpath=`pwd`

for dep in `find . -maxdepth 1 -type d`; do
	if [ '.' == $dep ]; then continue; fi
	if [ '..' == $dep ]; then continue; fi

	cd $dep
	echo "enter: $dep"
	rm -rf __zjms__ 2>/dev/null

	cd $curpath
done
