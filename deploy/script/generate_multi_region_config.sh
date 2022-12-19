set -e

config_path=$1
output_path=$2

files=`ls ${config_path}`

cd ${output_path}

CERT_TOOLS_BIN=${BAZEL_WORKSPACE_PATH}/bazel-bin/tools/certificate_tools
ADMIN_PRIVATE_KEY=${BAZEL_WORKSPACE_PATH}/cert/admin.key.pri
ADMIN_PUBLIC_KEY=${BAZEL_WORKSPACE_PATH}/cert/admin.key.pub
CONFIG_TOOLS_BIN=${BAZEL_WORKSPACE_PATH}/bazel-bin/tools/generate_mulregion_config
CONFIG_KEY_SH=${BAZEL_WORKSPACE_PATH}/deploy/script/generate_multi_region_key.sh
CERT_PATH=cert/
USERNAME=ubuntu
BASE_PORT=17000
CLIENT_NUM=1

bazel build //tools:certificate_tools
bazel build //tools:generate_mulregion_config

# ============  generate server keys (pub/pri) ==================
server_tot_num=0

for efile in ${files[@]};
do
file=${config_path}/$efile
echo $file
. ${file}

for _ in ${iplist[@]};
do
  server_tot_num=$(($server_tot_num+1))
done

done

echo "total:"${server_tot_num}

${CONFIG_KEY_SH} ${server_tot_num}  ${output_path}

# ============  generate region configs ==================
region_id=1
node_idx=1
server_config_list=()
for efile in ${files[@]};
do
file=${config_path}/$efile
echo $file
. ${file}

tot=0
for _ in ${iplist[@]};
do
  tot=$(($tot+1))
done

echo "node num:"$tot
echo "list num:"${#server_config_list[@]}

client_config=client_region${region_id}.config
server_config=server_region${region_id}.config
server_config_list[${#server_config_list[@]}]=$server_config
echo "" > $client_config
echo "" > $server_config
echo "list num:"${#server_config_list[@]}


idx=1
for ip in ${iplist[@]};
do
  port=$((${BASE_PORT}+${node_idx}))
  public_key=${CERT_PATH}/node_${node_idx}.key.pub 

  # create public key
  # create server config
  # create the public key and certificate
  if [ $(($idx+$CLIENT_NUM)) -gt $tot ] ; then
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${node_idx} ${ip} ${port} client
    echo "${node_idx} ${ip} ${port}" >> $client_config
  else
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${node_idx} ${ip} ${port} replica
    echo "${node_idx} ${ip} ${port}" >> $server_config
  fi

  node_idx=$(($node_idx+1))
  idx=$(($idx+1))
done

  region_id=$(($region_id+1))
done

echo "list:"${server_config_list[@]}
python3 ${CONFIG_TOOLS_BIN} ${server_config_list[@]} 
