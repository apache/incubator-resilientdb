#export server=//benchmark/protocols/pbft:kv_server_performance
#export COPTS="--define enable_leveldb=True"

. ./script/env.sh
./script/deploy.sh $1

. ./script/load_config.sh $1

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}
user=junchao
home_path="/users/junchao"

sleep 60

bazel run //benchmark/protocols/pbft:kv_service_tools 
for((i=1;;i++))
do
config_file=$PWD/config_out/client${i}.config
if [ ! -f "$config_file" ]; then
  break;
fi
echo "get cofigfile:"$config_file
${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools $config_file
done

echo "wait"
sleep 60

echo "benchmark done"
for ip in ${iplist[@]};
do
echo "$ip"
`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ${user}@${ip} "killall -9 ${server_bin}"` & 
done

wait

i=0
echo "getting results"
for ip in ${iplist[@]};
do
  i=`expr $i + 1`
  echo "scp -i ${key} ${user}@${ip}:${home_path}/${server_bin}.log ./${ip}_log"
  `scp -i ${key} ${user}@${ip}:${home_path}/${server_bin}.log result_${i}_log` &
done

wait

python3 performance/calculate_result.py `ls result_*_log` > results.log

rm -rf result_*_log
echo "save result to results.log"
cat results.log
