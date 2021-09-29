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
echo "Throughputs:"
for i in $(seq 0 $(($total_nodes))); do
	if [ "$i" -lt "$snodes" ]; then
		temp=$(tail -20 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 'tput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		temp3=$(tail -20 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 'cput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		temp2=$(tail -74 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 'msg_send_cnt=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		if [ ! -z "$temp" ]; then
			avg_thp=$(($avg_thp + $temp))
			thp_cnt=$(($thp_cnt + 1))
			shard_counter=$(($shard_counter + 1))
			shard_tp=$(($shard_tp + $temp))
			shard_ctp=$(($shard_ctp + $temp3))
			#avg_msg=$(($avg_msg + $temp2))
			#msg_cnt=$(($msg_cnt + 1))
		fi
		if [[ $shard_counter -eq $shard_size ]]; then
			shards_tp=("${shards_tp[@]}" "$shard_tp")
			shards_ctp=("${shards_ctp[@]}" "$shard_ctp")
			shard_counter=0
			shard_tp=0
			shard_ctp=0
		fi
		j=$i
		if [ $j -lt 10 ]; then
			j="0$j"
		fi
		echo -e "$j: ${Red}${temp}${Nc}\t${Red}${temp3}${Nc}"
	else
		temp=$(tail -4 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 'tput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		client_thp=$(($client_thp + $temp))
		j=$i
		if [ $j -lt 10 ]; then
			j="0$j"
		fi
		echo -e "$j: ${Red}${temp}${Nc}"
	fi

done

echo "Latencies:"
for i in $(seq $snodes $(($total_nodes))); do
	temp=$(tail -11 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 'AVG: .{1,13}' | grep -o ${flags} '\d+\.\d+')
	if [ ! -z "$temp" ]; then
		avg_lt=$(echo "$avg_lt + $temp" | bc)
		lt_cnt=$(($lt_cnt + 1))
		echo -e "latency $i: ${Red}${temp}${Nc}"
	else
		echo -e "latency $i: ${Red}NAAAAAAN${Nc}"
	fi

done

echo

echo "idle times:"
for i in $(seq 0 $(($snodes - 1))); do
	echo "I Node: $i"
	times=($(tail -50 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} "idle_time_worker.{1,10}" | grep -o ${flags} '\d+\.\d+'))
	for ((j = 0; j < ${#times[@]}; j++)); do
		# echo -e "Worker THD ${j}: ${Red}${times[$j]}${Nc}"
		echo -en "${Red}${times[$j]}${Nc}    "
	done
	echo
done

echo "Memory:"
for i in $(seq 0 $total_nodes); do
	lines=10
	if [  $i -lt $snodes ]; then
		lines=20
	fi
	mem=$(tail -${lines} s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 's_mem_usage=.{1,7}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
	[ ! -z "$mem" ] || mem=0
	echo "$i: $(($mem / 1000)) MB"
done
echo

total_tp=0
avg_ctp=0
counter=0
echo "Shards TP:"
for t in ${shards_tp[@]}; do
	shard_avg_tp=$(echo "$t / $shard_size" | bc)
	total_tp=$(($total_tp + $shard_avg_tp))
	echo "Shards ${counter} TP: $(echo -e ${Red}${shard_avg_tp}${Nc})"
	counter=$(($counter + 1))
done
counter=0
echo "Shards CTP:"
for t in ${shards_ctp[@]}; do
	shard_avg_tp=$(echo "$t / $shard_size" | bc)
	avg_ctp=$(($avg_ctp + $shard_avg_tp))
	echo "Shards ${counter} CTP: $(echo -e ${Red}${shard_avg_tp}${Nc})"
	if [ $shard_avg_tp -gt 0 ]; then
		counter=$(($counter + 1))
	fi

done
echo
if [ $counter -eq 0 ]; then
	counter=1
fi
echo "total avg TP:  $(echo -e ${Red}${total_tp}${Nc})"
echo "total avg CTP: $(echo -e ${Red}$(echo "scale=0;$avg_ctp / $counter" | bc)${Nc})"
echo ""
echo "Final avg TP: $(echo -e ${Red}$(echo "scale=0;$avg_ctp / $counter + $total_tp" | bc)${Nc})"
echo "avg lt  ${lt_cnt}: $(echo -e ${Red}$(echo "scale=3;$avg_lt / $lt_cnt" | bc)${Nc})"
#echo "avg msg: ${msg_cnt}: $(echo -e ${Red}$(expr $avg_msg / $msg_cnt)${Nc})"
# echo -e "cli thp sum: ${RED}$client_thp${NC}"
