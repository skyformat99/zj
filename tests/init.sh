#!/usr/bin/env bash

sed -i.bak '/export ZJMS_PATH=/d' ~/.bashrc
sed -i.bak '/export LD_LIBRARY_PATH=/d' ~/.bashrc

echo "export ZJMS_PATH=${HOME}/zjms" >> ~/.bashrc

perl -ane 'print "$F[1]:" if /^-L/' ${HOME}/zjms/ccpath >/tmp/__z__ldpath
echo "export LD_LIBRARY_PATH=`cat /tmp/__z__ldpath`$LD_LIBRARY_PATH" >> ~/.bashrc

bash ~/.bashrc

curdir=`pwd`

cd ../src
./init.sh
cd $curdir
