#!/bin/bash

G='\033[0;32m'
Y='\033[0;33m'
NC='\033[0m'

name=`date "+%F-%T"`
ccalgs="$*"
echo -e "${G}name: $name${NC}"
echo -e "${G}ccalgs: $*${NC}"

mkdir -p ./results/$name
exps=("tpcc-1" "tpcc-2" "tpcc-3" "tpcc-4" "tpcc-5" "tpcc-6" "ycsb-1" "ycsb-2" "ycsb-3" "ycsb-4")
for e in ${exps[@]}; do
    echo -e "${Y}experiment: $e${NC}"
    python $e.py $ccalgs | tee results/$name/$e.out
done
