#!/bin/bash

precentages=(0 5 10 20 40 80 100)

name="S"
runs=2
nodes=420
clients=75

for i in "${precentages[@]}"
do
    sed -i "22s/.*/#define CROSS_SHARD_PRECENTAGE ${i}/" config.h
	make clean;
    make -j8;
    ./scripts/startResilientDB.sh ${nodes} ${clients} ${name}_${i}p $runs
done
