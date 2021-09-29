#!/bin/bash
unset GREP_OPTIONS
# Black='\033[1;90m'      # Black
Nc='\033[0m'
Red='\033[1;91m' # Red

# Green='\033[1;92m'      # Green
# Yellow='\033[1;93m'     # Yellow
# Blue='\033[1;94m'       # Blue
# Purple='\033[1;95m'     # Purple
# Cyan='\033[1;96m'       # Cyan
# White='\033[1;97m'	  # White

snodes=$1
cnodes=$2
protocol=$3
shard_size=$4
bsize=$5
run=$6
folder=$7
total_nodes=$((cnodes + snodes - 1))
avg_thp=0
thp_cnt=0
client_thp=0
avg_lt=0.0
lt_cnt=0
avg_msg=0
msg_cnt=0
shard_counter=0
shard_tp=0
shard_ctp=0
shards_tp=()
shards_ctp=()
if [ "$(uname)" == "Darwin" ]; then
	flags="-E"
else
	flags="-P"
fi

cd results/
#if [ ! -z "$folder" ];then
#    cd $folder
#fi

echo "Outputs - Inputs"
for i in $(seq 0 $(($snodes - 1))); do
	echo "I Node Output: $i"
	times=($(tail -70 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} "Output .{1,10}: .{1,6}" | grep -o ${flags} '\d+\.\d+'))
	for ((j = 0; j < ${#times[@]}; j++)); do
		# echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
		echo -en "${Red}${times[$j]}${Nc}    "
	done
	echo -en " ----     "
	times=($(tail -70 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} "Input .{1,10}: .{1,6}" | grep -o ${flags} '\d+\.\d+'))
	for ((j = 0; j < ${#times[@]}; j++)); do
		# echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
		echo -en "${Red}${times[$j]}${Nc}    "
	done
	echo
done
# echo "Inputs:"
# for i in $(seq 0 $(($snodes - 1))); do
# 	echo "I Node Input: $i"
# 	times=($(tail -70 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} "Input .{1,10}: .{1,6}" | grep -o ${flags} '\d+\.\d+'))
# 	for ((j = 0; j < ${#times[@]}; j++)); do
# 		# echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
# 		echo -en "${Red}${times[$j]}${Nc}    "
# 	done
# 	echo
# done