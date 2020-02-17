#!/bin/bash

home_directory="/home/expo"
nodes=$1
name=$2
result_dir=$3
input="./ifconfig.txt"
i=0
while IFS= read -r line
do
	cmd="scp expo@${line}:${home_directory}/resilientdb/${name}*.out ${result_dir}"
	echo "$cmd"
	$($cmd) &
	i=$(($i+1))
done < "$input"
wait

i=0
while IFS= read -r line
do
	cmd="ssh expo@${line} rm ${home_directory}/resilientdb/*.out  & rm ${home_directory}/resilientdb/monitor/*.out"
	$($cmd) &
	i=$(($i+1))
done < "$input"
wait