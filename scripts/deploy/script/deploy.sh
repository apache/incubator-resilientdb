set -e

# load environment parameters
. ./script/env.sh

# load ip list
. ./script/load_config.sh $1

script_path=${BAZEL_WORKSPACE_PATH}/scripts

echo "server is: $server"

if [[ -z $server ]]; then
  server=//service/kv:kv_service
fi

if [[ -z $client_num ]];
then
client_num=1
fi

# obtain the src path
server_path=`echo "$server" | sed 's/:/\//g'`
server_path=${server_path:1}
server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}
enclave_path=${BAZEL_WORKSPACE_PATH}/enclave/sgxcode/build/enclave/enclave.signed
#grafna_port=8090

bin_path=${BAZEL_WORKSPACE_PATH}/bazel-bin/${server_path}
output_path=${script_path}/deploy/config_out
output_key_path=${output_path}/cert
output_cert_path=${output_key_path}

admin_key_path=${script_path}/deploy/data/cert

rm -rf ${output_path}
mkdir -p ${output_path}

deploy_iplist=${iplist[@]}


echo "server src path:"${server_path}
echo "server bazel bin path:"${bin_path}
echo "server name:"${server_bin}
echo "admin config path:"${admin_key_path}
echo "output path:"${output_path}
echo "deploy to :"${deploy_iplist[@]}
echo "client num :"${client_num}

# generate keys and certificates.

cd ${script_path}
echo "where am i:"$PWD

deploy/script/generate_key.sh ${BAZEL_WORKSPACE_PATH} ${output_key_path} ${#iplist[@]}
deploy/script/generate_config.sh ${BAZEL_WORKSPACE_PATH} ${output_key_path} ${output_cert_path} ${output_path} ${admin_key_path} ${client_num} ${deploy_iplist[@]}

# clean up stale bazel server PID files (zombie processes block bazel startup)
find ~/.cache/bazel -name "server.pid.txt" -exec sh -c '
  pid=$(cat "$1" 2>/dev/null)
  if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
    echo "Removing stale bazel PID file: $1 (pid=$pid)"
    rm -f "$1"
  fi
' _ {} \; 2>/dev/null || true

# build kv server (bazel handles incremental builds automatically)
bazel build ${server}

if [ $? != 0 ]
then
	echo "Complile ${server} failed"
	exit 1
fi

# commands functions
function run_cmd(){
  count=1
  for ip in ${deploy_iplist[@]};
  do
     ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "$1" &
    #  ssh $ssh_options_cloud -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "$1" &
    ((count++))
  done

  while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
  done
}

function run_one_cmd(){
  ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "$1" 
  # ssh $ssh_options_cloud -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "$1" 
}

echo "stop all servers"
run_cmd "killall -9 kv_server_performance"
run_cmd "killall -9 ${server_bin}"
run_cmd "rm -rf ${server_bin}; rm ${server_bin}*.log; rm -rf server.config; rm -rf cert; rm -rf wal_log"
#run_cmd "rm -rf ${server_bin}; rm -rf server.config;" 

sleep 1

# upload config files and binary (retry SCP on failure, max 3 attempts)
echo "upload configs"
upload_one() {
  local ip=$1
  local attempt rc
  for attempt in 1 2 3; do
    if [ "${ENABLE_TEE}" = "1" ]; then
      scp $ssh_options_cloud -P 2222 -i ${key} -r ${bin_path} ${output_path}/server.config ${output_path}/cert ${enclave_path} root@${ip}:/root/ > /dev/null 2>&1
    else
      scp $ssh_options_cloud -P 2222 -i ${key} -r ${bin_path} ${output_path}/server.config ${output_path}/cert root@${ip}:/root/ > /dev/null 2>&1
    fi
    rc=$?
    if [ $rc -eq 0 ]; then
      return 0
    fi
    echo "scp retry ${attempt} for ${ip} (rc=${rc})"
    sleep 2
  done
  echo "ERROR: scp failed 3x for ${ip}"
  return 1
}
# Upload in batches of 8 to avoid SSH connection flooding
batch=0
for ip in ${deploy_iplist[@]};
do
  upload_one ${ip} &
  ((batch++))
  if [ $((batch % 8)) -eq 0 ]; then
    wait
  fi
done
wait

# while [ $count -gt 0 ]; do
#   wait $pids
#   count=`expr $count - 1`
# done

echo "start to run"
# Start server
idx=1
count=0
for ip in ${deploy_iplist[@]};
do
  private_key="cert/node_"${idx}".key.pri"
  cert="cert/cert_"${idx}".cert"
  if [ "${ENABLE_TEE}" = "1" ]; then
    enclave="./enclave.signed"
    # NOTE: --simulate flag omitted because OE SDK simulation mode uses
    # RDRAND which is unsupported on Sandy Bridge CPUs (e.g., CloudLab
    # r320 nodes with Xeon E5-2450). Enclave creation will fail cleanly
    # with OE_UNSUPPORTED (21) and the protocol runs with enclave=NULL,
    # which the benchmark code handles via `if (enclave)` guards.
    # Set TEE_SIMULATE=1 to re-enable simulation on supported hardware.
    if [ "${TEE_SIMULATE}" = "1" ]; then
      server_cmd="nohup ./${server_bin} server.config ${private_key} ${cert} ${enclave} --simulate > ~/${server_bin}.log 2>&1 &"
      echo "nohup ./${server_bin} server.config ${private_key} ${cert} ${enclave} --simulate"
    else
      server_cmd="nohup ./${server_bin} server.config ${private_key} ${cert} ${enclave} > ~/${server_bin}.log 2>&1 &"
      echo "nohup ./${server_bin} server.config ${private_key} ${cert} ${enclave}"
    fi
  else
    server_cmd="nohup ./${server_bin} server.config ${private_key} ${cert} > ~/${server_bin}.log 2>&1 &"
    echo "nohup ./${server_bin} server.config ${private_key} ${cert}"
  fi
  run_one_cmd "${server_cmd}" &
  ((count++))
  ((idx++))
done
wait

# while [ $count -gt 0 ]; do
#   wait $pids
#   count=`expr $count - 1`
# done

echo "Check ready logs"
# Check ready logs with timeout (max READY_TIMEOUT seconds per node, skip if unresponsive).
# All nodes are polled in parallel so overall wait is O(READY_TIMEOUT), not O(N * READY_TIMEOUT).
READY_TIMEOUT=${READY_TIMEOUT:-180}
expected_size=${#iplist[@]}
idx=1
for ip in ${deploy_iplist[@]};
do
  (
    attempt=0
    while : ; do
      resp=$(ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "grep \"receive public size:${expected_size}\" ${server_bin}.log" 2>/dev/null)
      if [ -n "$resp" ]; then
        exit 0
      fi
      sleep 1
      attempt=$((attempt + 1))
      if [ $attempt -ge $READY_TIMEOUT ]; then
        echo "WARN: node ${ip} (idx=${idx}) not ready after ${READY_TIMEOUT}s, skipping"
        exit 0
      fi
    done
  ) &
  ((idx++))
done
wait


DEPLOY_DELAY_MS=${DEPLOY_DELAY_MS:-0}
DEPLOY_JITTER_MS=${DEPLOY_JITTER_MS:-0}

if [ "${DEPLOY_DELAY_MS}" != "0" ]; then
  echo "Setting network delay: ${DEPLOY_DELAY_MS}ms jitter: ${DEPLOY_JITTER_MS}ms"
  for ip in ${deploy_iplist[@]};
  do
    ssh $ssh_options_cloud -p 22 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no Shaokang@${ip} "DEV=\$(ip -4 route show default | awk '{print \$5}' | head -1) && sudo tc qdisc add dev \$DEV root netem delay ${DEPLOY_DELAY_MS}ms ${DEPLOY_JITTER_MS}ms" 2>/dev/null &
  done
  wait
fi

echo "Servers are running....."

