#!/usr/bin/env sh

./init.sh

for unit in `find . -name "zj_*.sh"`; do
	($unit 2>/dev/null) &
done
