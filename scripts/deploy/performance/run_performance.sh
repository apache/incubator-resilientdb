#export COPTS="--define enable_leveldb=True"

. ./script/env.sh

./script/copy_local_db.sh

./script/deploy.sh $1

. ./script/load_config.sh $1

USER_NAME="ubuntu"

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

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

sleep 20

echo "benchmark done"
count=1
for ip in ${iplist[@]};
do
`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no $USER_NAME@${ip} "killall -9 ${server_bin}"` &
((count++))
done

while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
done

echo "getting results"
id=0
for ip in ${iplist[@]};
do
  id=`expr $id + 1`
  echo "scp -i ${key} $USER_NAME@${ip}:~/${server_bin}.log ./${ip}_log"
  `scp -i ${key} $USER_NAME@${ip}:~/${server_bin}.log result_${id}_log` &
done

wait

python3 performance/calculate_result.py `ls result_*_log` > results.log

# rm -rf result_*_log
echo "save result to results.log"
cat $TEMPLATE_PATH
cat results.log

