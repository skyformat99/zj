#!/usr/bin/env bash

./init.sh

cd ../src
curdir=`pwd`
for unit in `find ${curdir} -name "*_test.sh"`; do
    cd `dirname ${unit}`
    echo -e "\x1b[31;01m[*] ${unit}\x1b[00m"
    $unit
done
