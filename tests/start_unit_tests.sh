#!/usr/bin/env bash

./init.sh

cd ../src
curdir=`pwd`
for unit in `find ${curdir} -name "*_test.sh"`; do
	cd `dirname ${unit}`
    $unit || exit 1
done
