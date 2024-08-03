export server=//benchmark/protocols/ooo_hs:kv_server_performance
export TEMPLATE_PATH=$PWD/config/ooo_hs.config

. ./script/env.sh

./script/deploy.sh $1

. ./script/load_config.sh $1

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

bazel run //benchmark/protocols/pbft:kv_service_tools 
echo "path:"${BAZEL_WORKSPACE_PATH}, "PWD":$PWD

for((i=1;;i++))
do
config_file=$PWD/config_out/client${i}.config
if [ ! -f "$config_file" ]; then
  break;
fi
echo "get cofigfile:"$config_file, "workspace:"${BAZEL_WORKSPACE_PATH}
${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools $config_file
done

sleep 60

echo "benchmark done"
count=1
for ip in ${iplist[@]};
do
`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ${USERNAME}@${ip} "killall -9 ${server_bin}"` 
((count++))
done

while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
done

count=1
echo "getting results"
for ip in ${iplist[@]};
do
  # if [ "$count" -gt 128 ]; then
  #     break
  # fi
  echo "scp -i ${key} ubuntu@${ip}:/home/ubuntu/${server_bin}.log ./${count}_log" 
  `scp -i ${key} ubuntu@${ip}:/home/ubuntu/${server_bin}.log result_${count}_log` &
  count=`expr $count + 1`
done

wait

python3 performance/calculate_result.py `ls result_*_log` > results.log

rm -rf result_*_log
echo "save result to results.log"
cat results.log