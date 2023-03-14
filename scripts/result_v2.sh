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
c_shard_size=$(echo "$snodes / $shard_size" | bc)
c_shard_size=$(echo "$cnodes / $c_shard_size" | bc)
bsize=$5
run=$6
folder=$7
total_nodes=$((cnodes + snodes - 1))
avg_lt=0.0
lt_cnt=0
avg_clt=0
clt_cnt=0
avg_lt_cnt=0
avg_msg=0
msg_cnt=0

shard_server_counter=0
shard_server_tp=0
shard_server_ctp=0
shards_server_tp=()
shards_server_ctp=()

shard_cl_counter=0
shard_cl_tp=0
shard_cl_ctp=0
shards_cl_tp=()
shards_cl_ctp=()

shard_lt=0
shard_clt=0
avg_shard_lt=0
shards_lt=()
shards_clt=()
avg_shards_lt=()
if [ "$(uname)" == "Darwin" ]; then
	flags="-E"
else
	flags="-P"
fi

if [ ! -z "$folder" ]; then
	cd $folder
else
	cd results/
fi
echo "Throughputs:"
for i in $(seq 0 $(($total_nodes))); do
	if [ "$i" -lt "$snodes" ]; then
		temp=$(tail -10 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^tput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		temp3=$(tail -10 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^cput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		# temp2=$(tail -74 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 'msg_send_cnt=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		if [ -z "$temp" ]; then
			temp=0
		fi
		if [ -z "$temp3" ]; then
			temp3=0
		fi
		shard_server_counter=$(($shard_server_counter + 1))
		shard_server_tp=$(($shard_server_tp + $temp))
		shard_server_ctp=$(($shard_server_ctp + $temp3))
		#avg_msg=$(($avg_msg + $temp2))
		#msg_cnt=$(($msg_cnt + 1))
		if [[ $shard_server_counter -eq $shard_size ]]; then
			shards_server_tp=("${shards_server_tp[@]}" "$shard_server_tp")
			shards_server_ctp=("${shards_server_ctp[@]}" "$shard_server_ctp")
			shard_server_counter=0
			shard_server_tp=0
			shard_server_ctp=0
		fi
		j=$i
		if [ $j -lt 10 ]; then
			j="0$j"
		fi
		echo -e "$j: ${Red}${temp}${Nc}\t${Red}${temp3}${Nc}"
	else
		temp=$(tail -10 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^tput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		temp2=$(tail -10 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^cput=.{1,13}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
		if [ -z "$temp" ]; then
			temp=0
		fi
		if [ -z "$temp2" ]; then
			temp2=0
		fi

		shard_cl_counter=$(($shard_cl_counter + 1))
		shard_cl_tp=$(($shard_cl_tp + $temp))
		shard_cl_ctp=$(($shard_cl_ctp + $temp2))
		#avg_msg=$(($avg_msg + $temp2))
		#msg_cnt=$(($msg_cnt + 1))
		if [[ $shard_cl_counter -eq $c_shard_size ]]; then
			shards_cl_tp=("${shards_cl_tp[@]}" "$shard_cl_tp")
			shards_cl_ctp=("${shards_cl_ctp[@]}" "$shard_cl_ctp")
			shard_cl_counter=0
			shard_cl_tp=0
			shard_cl_ctp=0
		fi

		j=$i
		if [ $j -lt 10 ]; then
			j="0$j"
		fi
		echo -e "$j: ${Red}${temp}${Nc}\t${Red}${temp2}${Nc}"
	fi

done

echo "Latencies:"
for i in $(seq $snodes $(($total_nodes))); do
	temp=$(tail -11 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^Latency=.{1,13}' | grep -o ${flags} '\d+\.\d+')
	temp2=$(tail -11 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^CLatency=.{1,13}' | grep -o ${flags} '\d+\.\d+')
	temp3=$(tail -50 s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} '^AVG Latency:.{1,13}' | grep -o ${flags} '\d+\.\d+')
	if [ ! -z "$temp" ]; then
		shard_lt=$(echo "scale=3;$shard_lt + $temp" | bc)
		lt_cnt=$(($lt_cnt + 1))
	else
		temp="NAN"
	fi
	if [ ! -z "$temp2" ]; then
		shard_clt=$(echo "scale=3;$shard_clt + $temp2" | bc)
		clt_cnt=$(($clt_cnt + 1))
	else
		temp2="NAN"
	fi
	if [ ! -z "$temp3" ]; then
		avg_shard_lt=$(echo "scale=3;$avg_shard_lt + $temp3" | bc)
		avg_lt_cnt=$(($avg_lt_cnt + 1))
	else
		temp3="NAN"
	fi
	shard_cl_counter=$(($shard_cl_counter + 1))
	echo -e "$i: ${Red}${temp}${Nc}\t${Red}${temp2}${Nc}"
	if [[ $shard_cl_counter -eq $c_shard_size ]]; then
		if [ $lt_cnt -ne 0 ]; then
			t=$(echo "scale=3;$shard_lt / $lt_cnt" | bc)
		else
			t=0
		fi
		if [ $clt_cnt -ne 0 ]; then
			t2=$(echo "scale=3;$shard_clt / $clt_cnt" | bc)
		else
			t2=0
		fi
		if [ $avg_lt_cnt -ne 0 ]; then
			t3=$(echo "scale=3;$avg_shard_lt / $avg_lt_cnt" | bc)
		else
			t3=0
		fi
		shards_lt=("${shards_lt[@]}" "$t")
		shards_clt=("${shards_clt[@]}" "$t2")
		avg_shards_lt=("${avg_shards_lt[@]}" "$t3")
		shard_cl_counter=0
		shard_lt=0
		shard_clt=0
		avg_shard_lt=0
		lt_cnt=0
		clt_cnt=0
		avg_lt_cnt=0
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
	if [ $i -lt $snodes ]; then
		lines=20
	fi
	mem=$(tail -${lines} s${snodes}_c${cnodes}_results_${protocol}_b${bsize}_run${run}_node${i}.out | grep -o ${flags} 's_mem_usage=.{1,7}' | grep -o ${flags} '=\d+' | grep -o ${flags} '\d+')
	[ ! -z "$mem" ] || mem=0
	echo "$i: $(echo "$mem / 1000" | bc)"
done
echo

# ----------------------------------- Servers
total_server_tp=0
avg_server_ctp=0
counter=0
echo "Shards Servers TP:"
for t in ${shards_server_tp[@]}; do
	shard_avg_tp=$(echo "$t / $shard_size" | bc)
	total_server_tp=$(($total_server_tp + $shard_avg_tp))
	echo "${counter}: $(echo -e ${Red}${shard_avg_tp}${Nc})"
	counter=$(($counter + 1))
done
counter=0
echo "Shards Servers CTP:"
for t in ${shards_server_ctp[@]}; do
	shard_avg_tp=$(echo "$t / $shard_size" | bc)
	avg_server_ctp=$(($avg_server_ctp + $shard_avg_tp))
	echo "${counter}: $(echo -e ${Red}${shard_avg_tp}${Nc})"
	if [ $shard_avg_tp -gt 0 ]; then
		counter=$(($counter + 1))
	fi

done
echo
if [ $counter -eq 0 ]; then
	counter=1
fi
echo "total avg servers TP:  $(echo -e ${Red}${total_server_tp}${Nc})"
echo "total avg servers CTP: $(echo -e ${Red}$(echo "scale=0;$avg_server_ctp / $counter" | bc)${Nc})"
echo "Final avg servers TP: $(echo -e ${Red}$(echo "scale=0;$avg_server_ctp / $counter + $total_server_tp" | bc)${Nc})"
echo "--------------------------------------------"
# ----------------------------------- Clients
total_cl_tp=0
total_cl_ctp=0
counter=0
echo -e "\nShards Clients TP:"
for t in ${shards_cl_tp[@]}; do
	total_cl_tp=$(($total_cl_tp + $t))
	echo -e "${counter}: $(echo -e ${Red}${t}${Nc})"
	counter=$(($counter + 1))
done
counter=0
echo "Shards Clients CTP:"
for t in ${shards_cl_ctp[@]}; do
	total_cl_ctp=$(($total_cl_ctp + ${t}))
	echo -e "${counter}: $(echo -e ${Red}${t}${Nc})"
	counter=$(($counter + 1))

done

echo "total avg clients TP:  $(echo -e ${Red}${total_cl_tp}${Nc})"
echo "total avg clients CTP: $(echo -e ${Red}${total_cl_ctp}${Nc})"
echo "Final avg clients TP: $(echo -e ${Red}$(echo "scale=0;$total_cl_tp + $total_cl_ctp" | bc)${Nc})"
echo "--------------------------------------------"
# ----------------------------------- Latency
avg_latency=0
avg_latency_cnt=0
avg_clatency=0
avg_clatency_cnt=0
avg_avg_latency=0
avg_avg_latency_cnt=0
echo -e "\nShards Latency:"
for t in ${shards_lt[@]}; do
	if [ ! "$t" = "0" ]; then
		avg_latency=$(echo "$avg_latency + $t" | bc)
		echo -e "${avg_latency_cnt}: $(echo -e ${Red}${t}${Nc})"
		avg_latency_cnt=$(($avg_latency_cnt + 1))
	else
		echo -e "${avg_latency_cnt}: $(echo -e ${Red}NAN${Nc})"
	fi
done
echo "Shards Cross Latency:"
for t in ${shards_clt[@]}; do
	if [ ! "$t" = "0" ]; then
		avg_clatency=$(echo "$avg_clatency + ${t}" | bc)
		echo -e "${avg_clatency_cnt}: $(echo -e ${Red}${t}${Nc})"
		avg_clatency_cnt=$(($avg_clatency_cnt + 1))
	else
		echo -e "${avg_clatency_cnt}: $(echo -e ${Red}NAN${Nc})"
	fi
done

echo "Shards AVG Latency:"
for t in ${avg_shards_lt[@]}; do
	if [ ! "$t" = "0" ]; then
		avg_avg_latency=$(echo "$avg_avg_latency + ${t}" | bc)
		echo -e "${avg_avg_latency_cnt}: $(echo -e ${Red}${t}${Nc})"
		avg_avg_latency_cnt=$(($avg_avg_latency_cnt + 1))
	else
		echo -e "${avg_avg_latency_cnt}: $(echo -e ${Red}NAN${Nc})"
	fi
done

echo "total avg Intra Latency:  $(echo -e ${Red}$(echo "scale=3;$avg_latency / $avg_latency_cnt" | bc)${Nc})"
if [ $avg_clatency_cnt -ne 0 ]; then
	echo "total avg Cross Latency: $(echo -e ${Red}$(echo "scale=3;$avg_clatency / $avg_clatency_cnt" | bc)${Nc})"
fi
echo "Final avg Latency: $(echo -e ${Red}$(echo "scale=3;($avg_latency + $avg_clatency) / ($avg_clatency_cnt + $avg_latency_cnt)" | bc)${Nc})"
