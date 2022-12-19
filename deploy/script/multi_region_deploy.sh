#set -x
set -e

# load environment parameters
. ./script/env.sh

binary_config=$(readlink -f $1)
# load server name and ssh key
. ${binary_config}
config_real_path=$(readlink -f $2) 
config_real_dir=$(dirname ${config_real_path})

output_path=${config_real_dir}/output
rm -rf ${output_path}
mkdir -p ${output_path}

# obtain the src path
server_path=`echo "$server" | sed 's/:/\//g'`
server_path=${server_path:1}
server_name=`echo "$server" | awk -F':' '{print $NF}'`
server_bin=${server_name}

bin_path=bazel-bin/${server_path}

echo "server src path:"${server_path}
echo "server bazel bin path:"${bin_path}
echo "server name:"${server_bin}
echo "config path:"${config_real_path}
echo "output path:"${output_path}

# generate keys and certificates.

cd ${BAZEL_WORKSPACE_PATH}
echo "where am i:"$PWD
deploy/script/generate_multi_region_config.sh ${config_real_path} ${output_path}

# build kv server
bazel build ${server}

# commands functions


function get_iplist() {
	file=$1
	echo "get file name:"${file}
	. ${file}	

	deploy_iplist=
	if [ ! -n $public_iplist ]; then
		echo " public ip is not set, use default one"
		deploy_iplist=${iplist[@]}
	else
		echo " public ip has been set, use public ip to deploy "
		deploy_iplist=${public_iplist[@]}
	fi       
	echo ${deploy_iplist[*]}
}

function wait_pid() {
	count=$1
	while [ $count -gt 0 ]; do
	  wait $pids
	  count=`expr $count - 1`
	done
}

function kill_bin(){
	get_iplist $1
	count=1
	for ip in ${deploy_iplist[@]};
	do
	   ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "killall -9 ${server_bin}" &
	  ((count++))
	done
	wait_pid $count
}

function rm_files(){
	get_iplist $1
	count=1
	for ip in ${deploy_iplist[@]};
	do
	   ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "rm -rf ${server_bin}; rm ${server_bin}*.log; rm -rf server.config; rm -rf cert;" &
	  ((count++))
	done
	wait_pid $count
}



function upload_files(){
	get_iplist $1
	echo "upload deploy list:"${deploy_iplist[@]}
	# upload config files and binary
	count=1
	for ip in ${deploy_iplist[@]};
	do
	  scp -i ${key} -r ${bin_path} $2 ${output_path}/cert ubuntu@${ip}:/home/ubuntu/ &
	  ((count++))
	done
	wait_pid $count
}

node_idx=1
function start_server() {
	get_iplist $1
	echo "start deploy list:"${deploy_iplist[@]}
	count=1
	for ip in ${deploy_iplist[@]};
	do
	  private_key="cert/node_"${node_idx}".key.pri"
	  cert="cert/cert_"${node_idx}".cert"
	  cmd="nohup ./${server_bin} $2 ${private_key} ${cert}  > ${server_bin}.log 2>&1 &"
	  echo "cmd:"$cmd
	  ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "$cmd"  &
	  ((count++))
	  ((node_idx++))
	done

	wait_pid $count
}



files=`ls ${config_real_path}`
region_id=1
for efile in ${files[@]};
do
file=${config_real_path}/$efile

kill_bin ${file} 
rm_files ${file}

# upload config files and binary
upload_files ${file} ${output_path}/server_region${region_id}.config_json
((region_id++))

echo "upload files done"

done

# Start server
node_idx=1
region_id=1
for efile in ${files[@]};
do
file=${config_real_path}/$efile
start_server ${file} server_region${region_id}.config_json
((region_id++))
done

echo "start server done, begin to check running"


# Check ready logs
idx=1
for ip in ${deploy_iplist[@]};
do
  resp=""
  while [ "$resp" = "" ]
  do
    resp=`ssh -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no ubuntu@${ip} "grep \"receive public size:${node_idx}\" ${server_bin}.log"` 
    if [ "$resp" = "" ]; then
      sleep 1
    fi
  done
  ((idx++))
done

echo "Servers are running....."

