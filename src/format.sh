#!/usr/bin/env bash

export LC_ALL="en_US.UTF-8"

for file in `find . -type f`; do
    perl -p -i -e 's/\t/    /g' $file
    perl -p -i -e 's/ +$//g' $file
done
