#!/bin/sh
make
./plot $1 $2 | python3 plot.py
