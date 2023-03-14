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
bsize=$4
run=$5
folder=$6
total_nodes=$((cnodes + snodes - 1))
avg_thp=0
thp_cnt=0
client_thp=0
avg_lt=0.0
lt_cnt=0
avg_msg=0
msg_cnt=0
if [ "$(uname)" == "Darwin" ]; then
	flags="-E"
else
	flags="-P"
fi

cd results/
# if [ ! -z "$folder" ]; then
# 	cd ../$folder
# 	echo "hi"
# fi
RES_OUT=()
echo "Throughputs:"
for i in $(seq 0 $(($total_nodes))); do
	if [ "$i" -lt "$snodes" ]; then
		RES_OUT[$i]=$(tail -100 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | awk 'p; /=======================================================/ {p=1}')
		temp=$(echo ${RES_OUT[$i]} | grep -o ${flags} 'tput[\s]+=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		if [ ! -z "$temp" ]; then
			avg_thp=$(($avg_thp + $temp))
			thp_cnt=$(($thp_cnt + 1))
		fi
	else
		RES_OUT[$i]=$(tail -70 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | awk 'p; /CLatency/ {p=1}')
		temp=$(echo ${RES_OUT[$i]} | grep -o ${flags} 'tput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+' | tail -1)
		client_thp=$(($client_thp + $temp))
	fi
	echo -e "$i: ${Red}${temp}${Nc}"
done

echo "Latencies:"
for i in $(seq $snodes $(($total_nodes))); do
	temp=$(echo ${RES_OUT[$i]} | grep -o ${flags} 'Latency=.{1,13}' | grep -o ${flags} '\d+\.\d+' | tail -1)
	avg_lt=$(echo "$avg_lt + $temp" | bc)
	lt_cnt=$(($lt_cnt + 1))
	echo -e "latency $i: ${Red}${temp}${Nc}"
done

echo

echo "idle times:"
for i in $(seq 0 $(($snodes - 1))); do
	echo "Idleness of node: $i"
	times=($(echo ${RES_OUT[$i]} | grep -o ${flags} "idle_time_worker.{1,10}" | grep -o ${flags} '\d+\.\d+'))
	exec_thread="$((${#times[@]} - 2))"
	for ((j = 0; j < ${#times[@]}; j++)); do
		if [ $j -gt 0 -a $j -lt $exec_thread ]; then
			if [ $i -eq 0 ]; then
				echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
			fi
		else
			echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
		fi
	done
done

# echo "Memory:"
# for i in $(seq 0 $total_nodes); do
# 	mem=$(echo ${RES_OUT[$i]} | grep -o ${flags} 's_mem_usage=.{1,7}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
# 	[ ! -z "$mem" ] || mem=0
# 	echo "$i: $(($mem / 1000)) MB"
# done
echo

echo "avg thp: ${thp_cnt}: $(echo -e ${Red}$(expr $avg_thp / $thp_cnt)${Nc})"
echo "avg lt : ${lt_cnt}: $(echo -e ${Red}$(echo "scale=3;$avg_lt / $lt_cnt" | bc)${Nc})"
#echo "avg msg: ${msg_cnt}: $(echo -e ${Red}$(expr $avg_msg / $msg_cnt)${Nc})"
# echo -e "cli thp sum: ${RED}$client_thp${NC}"
