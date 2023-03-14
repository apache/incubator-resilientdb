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
if [ ! -z "$folder" ]; then
	cd ../$folder
	echo "hi"
fi
RES_OUT=()
TPS=()
for i in $(seq 0 $(($total_nodes))); do
	if [ "$i" -lt "$snodes" ]; then
		RES_OUT[$i]=$(tail -100 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | awk 'p; /FINISH/ {p=1}')
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
	TPS[$i]=${temp}
done
avg_tp=$(expr $avg_thp / $thp_cnt)
significant_diff=$(expr $avg_tp / 50) # 2 percent
echo "Replicas TP:"
for i in $(seq 0 $(($total_nodes))); do
	if [ $((${TPS[$i]} - $avg_tp)) -gt $significant_diff -o $(($avg_tp - ${TPS[$i]})) -gt $significant_diff ]; then
		echo -e "$i: ${Red}${TPS[$i]}${Nc}"
	fi
	if [ "$i" -eq $(($snodes-1)) ]; then
		echo "Client TP:"
	fi
done

echo "Latencies:"
for i in $(seq $snodes $(($total_nodes))); do
	temp=$(echo ${RES_OUT[$i]} | grep -o ${flags} 'Latency=.{1,13}' | grep -o ${flags} '\d+\.\d+' | tail -1)
	avg_lt=$(echo "$avg_lt + $temp" | bc)
	lt_cnt=$(($lt_cnt + 1))
	echo -e "latency $i: ${Red}${temp}${Nc}"
done

echo

echo "Primary idle times:"
times=($(echo ${RES_OUT[0]} | grep -o ${flags} "idle_time_worker.{1,10}" | grep -o ${flags} '\d+\.\d+'))
for ((j = 0; j < ${#times[@]}; j++)); do
	echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
done

for i in $(seq 0 $(($snodes - 1))); do
	wt_idle=$(echo ${RES_OUT[$i]} | grep -o ${flags} "idle_time_worker.{1,10}" | grep -o ${flags} '\d+\.\d+' | head -1)
	echo -e "WT0 of ${i}: ${Red}${wt_idle}${Nc}"
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
