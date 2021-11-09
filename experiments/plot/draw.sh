#!/bin/bash

for i in 1 2 3 4
do
    python draw.py ycsb-$i
done

for i in 1 2 3 4 6
do
    python draw.py tpcc-$i
done

python tpcc-5.py
