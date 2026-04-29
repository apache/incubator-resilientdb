#!/bin/bash
#export COPTS="--define enable_leveldb=True"

# Load environment (sets BAZEL_WORKSPACE_PATH, ssh_options_cloud, etc.)
. ./script/env.sh
. ./script/load_config.sh $1

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

for ip in ${iplist[@]};
do
timeout 15 ssh $ssh_options_cloud -o ConnectTimeout=10 -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "killall -9 ${server_bin}" 2>/dev/null &
done
wait


./script/deploy.sh $1

# Re-load config (deploy.sh may change CWD)
. ./script/load_config.sh $1

server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

[ -f "${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools" ] || bazel build //benchmark/protocols/pbft:kv_service_tools
${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools

for((i=1;;i++))
do
config_file=$PWD/config_out/client${i}.config
if [ ! -f "$config_file" ]; then
  break;
fi
echo "get cofigfile:"$config_file
timeout 30 ${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools $config_file || echo "client $i timed out"
done

BENCH_DURATION=${BENCH_DURATION:-60}
sleep $BENCH_DURATION

for ip in ${iplist[@]};
do
  timeout 15 ssh $ssh_options_cloud -o ConnectTimeout=10 -p 22 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no Shaokang@${ip} "DEV=\$(ip -4 route show default | awk '{print \$5}' | head -1) && sudo tc qdisc del dev \$DEV root" 2>/dev/null &
done
wait


echo "benchmark done"
for ip in ${iplist[@]};
do
timeout 15 ssh $ssh_options_cloud -o ConnectTimeout=10 -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "killall -9 ${server_bin}" 2>/dev/null &
done
wait

echo "getting results"
# Save raw replica logs to a timestamped dir so they can be re-analyzed
# with phase-aware metrics (normal_results/<server_name>_<timestamp>/)
RESULT_DIR="normal_results/${server_name}_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"

for ip in ${iplist[@]};
do
  timeout 30 scp $ssh_options_cloud -o ConnectTimeout=10 -P 2222 -i ${key} root@${ip}:/root/${server_bin}.log "${RESULT_DIR}/result_${ip}_log" 2>/dev/null &
done
wait

python3 performance/calculate_result.py "${RESULT_DIR}"/result_*_log > "${RESULT_DIR}/results.log"

# Keep a top-level results.log symlink for backward compatibility with
# wrappers that grep ./results.log
ln -sf "${RESULT_DIR}/results.log" results.log

echo "raw logs saved to: ${RESULT_DIR}"
echo "save result to results.log"
cat "${RESULT_DIR}/results.log"
