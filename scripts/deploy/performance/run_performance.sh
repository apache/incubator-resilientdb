#export COPTS="--define enable_leveldb=True"

. ./script/env.sh

./script/deploy.sh $1

. ./script/load_config.sh $1

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}
user=ubuntu
home_path="/home/ubuntu"

for ip in ${iplist[@]};
do
`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ${user}@${ip} "rm -rf local_ordering.txt; rm -rf final_ordering.txt"` & 
done

echo "deleting ordering txt files done."


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
  `scp -i ${key} ${user}@${ip}:${home_path}/local_ordering.txt local_ordering_${i}` &
  `scp -i ${key} ${user}@${ip}:${home_path}/final_ordering.txt final_ordering_${i}` &
done

wait

python3 performance/calculate_result.py `ls result_*_log` > results.log

# rm -rf result_*_log
echo "save result to results.log"
cat results.log
cat $TEMPLATE_PATH
echo $TEMPLATE_PATH