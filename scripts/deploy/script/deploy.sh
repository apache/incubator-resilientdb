set -e

# load environment parameters
. ./script/env.sh

# load ip list
. ./script/load_config.sh $1

script_path=${BAZEL_WORKSPACE_PATH}/scripts

if [[ -z $server ]];
then
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

# build kv server
bazel build ${server} 

if [ $? != 0 ]
then
	echo "Complile ${server} failed"
	exit 0
fi

# commands functions
function run_cmd(){
  count=1
  for ip in ${deploy_iplist[@]};
  do
     ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "$1" &
    ((count++))
  done

  while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
  done
}

function run_one_cmd(){
  ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "$1" 
}

run_cmd "killall -9 ${server_bin}"
run_cmd "rm -rf ${server_bin}; rm ${server_bin}*.log; rm -rf server.config; rm -rf cert; rm -rf wal_log"
#run_cmd "rm -rf ${server_bin}; rm -rf server.config;" 

sleep 1

# upload config files and binary
echo "upload configs"
count=0
for ip in ${deploy_iplist[@]};
do
  scp -i ${key} -r ${bin_path} ${BAZEL_WORKSPACE_PATH}/service/contract/benchmark/data/smallbank.json ${output_path}/server.config ${output_path}/cert ubuntu@${ip}:/home/ubuntu/  > null 2>&1 &
  #scp -i ${key} -r ${bin_path} ${output_path}/server.config ubuntu@${ip}:/home/ubuntu/  > null 2>&1 &
  ((count++))
done

while [ $count -gt 0 ]; do
  wait $pids
  count=`expr $count - 1`
done

echo "start to run"
# Start server
idx=1
count=0
for ip in ${deploy_iplist[@]};
do
  private_key="cert/node_"${idx}".key.pri"
  cert="cert/cert_"${idx}".cert"
  run_one_cmd "nohup ./${server_bin} server.config ${private_key} ${cert}  > ${server_bin}.log 2>&1 &" &
  ((count++))
  ((idx++))
done

while [ $count -gt 0 ]; do
  wait $pids
  count=`expr $count - 1`
done

function check(){
   server_ip=$1
   server_num=$2
  resp=""
  while [ "$resp" = "" ]
  do
    resp=`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${server_ip} "grep \"receive public size:${server_num}\" ${server_bin}.log"` 
    if [ "$resp" = "" ]; then
      sleep 1
    else
      echo "${server_ip} done"
    fi
  done
}

# Check ready logs
for ip in ${deploy_iplist[@]};
do
  check ${ip} ${#iplist[@]} &
done

wait

echo "Servers are running....."

