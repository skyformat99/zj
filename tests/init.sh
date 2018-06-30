#!/usr/bin/env bash

sed -i.bak '/export ZJ_PATH=/d' ~/.bashrc
sed -i.bak '/export LD_LIBRARY_PATH=/d' ~/.bashrc

echo "export ZJ_PATH=${HOME}/zj" >> ~/.bashrc

perl -ane 'print "$F[1]:" if /^-L/' ${HOME}/zj/ccpath >/tmp/__z__ldpath
echo "export LD_LIBRARY_PATH=`cat /tmp/__z__ldpath`$LD_LIBRARY_PATH" >> ~/.bashrc

bash ~/.bashrc

curdir=`pwd`

cd ../src
./init.sh
cd $curdir
