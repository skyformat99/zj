#!/usr/bin/env bash

curdir=`pwd`
cd ../deps
./BUILD.sh
cd $curdir

echo "#ifndef _ENV_H" > ./env.h
echo "#define _ENV_H" >> ./env.h

echo "#define _UNIT_TEST_USER \"`whoami`\"">>./env.h

echo "#endif //_ENV_H">> ./env.h
