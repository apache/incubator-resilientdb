export server=//benchmark/protocols/pbft:kv_server_performance

./script/deploy.sh $1

. ./script/load_config.sh $1

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

bazel run //benchmark/protocols/pbft:kv_service_tools -- $PWD/config_out/client.config 

sleep 60

echo "benchmark done"
count=1
for ip in ${iplist[@]};
do
`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "killall -9 ${server_bin}"` 
((count++))
done

while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
done


echo "getting results"
for ip in ${iplist[@]};
do
  echo "scp -i ${key} ubuntu@${ip}:/home/ubuntu/${server_bin}.log ./${ip}_log"
  `scp -i ${key} ubuntu@${ip}:/home/ubuntu/${server_bin}.log result_${ip}_log` 
done

python3 performance/calculate_result.py `ls result_*_log` > results.log

rm -rf result_*_log
echo "save result to results.log"
cat results.log
