#!/usr/bin/env bash

./init.sh

cd ../src
curdir=`pwd`
for unit in `find ${curdir} -name "*_test.sh"`; do
    cd `dirname ${unit}`
    echo "[*] ${unit}"
    $unit
done
