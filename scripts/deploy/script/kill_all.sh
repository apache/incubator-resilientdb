set -e

# load environment parameters
. ./script/env.sh

# load ip list
. ./script/load_config.sh $1

if [[ -z $server ]];
then
server=//service/kv:kv_service
fi

script_path=${BAZEL_WORKSPACE_PATH}/scripts
server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}
deploy_iplist=${iplist[@]}

echo "server name:"${server_bin}

# commands functions
function run_cmd(){
  count=1
  for ip in ${deploy_iplist[@]};
  do
     echo "kill from ${ip}"
     ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "$1" &
    ((count++))
  done

  while [ $count -gt 0 ]; do
        wait $pids
        count=`expr $count - 1`
  done
}

run_cmd "killall -9 ${server_bin}"
run_cmd "rm -rf ${server_bin}; rm ${server_bin}*.log; rm -rf server.config; rm -rf cert;"

echo "killall done"
