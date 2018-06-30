#!/usr/bin/env bash

for file in `find . -type f`; do 
    perl -p -i -e 's/\t/    /g' $file
done
