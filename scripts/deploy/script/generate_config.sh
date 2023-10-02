base_path=$1; shift
key_path=$1; shift
output_cert_path=$1; shift
output_path=$1; shift
admin_key_path=$1; shift
#client_num=$1; shift

iplist=$@
echo "iplist:"${iplist[@]}

echo "generage certificates"

echo "base path:"$base_path
echo "key path:"$key_path
echo "output cert path:"$output_cert_path
echo "output path:"$output_path
echo "admin_key_path:"$admin_key_path

cd ${output_path}

ADMIN_PRIVATE_KEY=${admin_key_path}/admin.key.pri
ADMIN_PUBLIC_KEY=${admin_key_path}/admin.key.pub

GEN_KEY_TOOLS=${base_path}/bazel-bin/tools/key_generator_tools
CERT_TOOLS_BIN=${base_path}/bazel-bin/tools/certificate_tools
CONFIG_TOOLS_BIN=${base_path}/bazel-bin/tools/generate_region_config

USERNAME=ubuntu
BASE_PORT=17000
CLIENT_NUM=1
#if [ $client_num -ne 0 ] ; then
#CLIENT_NUM=$client_num
#fi

echo "client num:"$CLIENT_NUM

echo "" > client.config
echo "" > server.config

bazel build //tools:key_generator_tools
bazel build //tools:certificate_tools
bazel build //tools:generate_region_config

idx=1
tot=0
for _ in ${iplist[@]};
do
  tot=$(($tot+1))
done

echo "node num:"$tot
echo "out cert path:"${output_cert_path}
mkdir -p ${output_cert_path}

for ip in ${iplist[@]};
do
  echo "get ip:"$ip, "idx:"$idx
  port=$((${BASE_PORT}+${idx}))
  public_key=${key_path}/node_${idx}.key.pub 

  if [ ! -f $public_key ]; then
    echo `${GEN_KEY_TOOLS} "${output_cert_path}/node_${idx}" "AES"`

    # create public key
    # create server config
    # create the public key and certificate
    if [ $idx -le $CLIENT_NUM ] ; then
      echo "gen client"
      $CERT_TOOLS_BIN ${output_cert_path} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} client
      echo "${idx} ${ip} ${port}" >> client.config
    else
      echo "gen server"
      $CERT_TOOLS_BIN ${output_cert_path} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} replica
      echo "${idx} ${ip} ${port}" >> server.config
    fi
  else
    if [ $idx -le $CLIENT_NUM ] ; then
      echo "add client"
      echo "${idx} ${ip} ${port}" >> client.config
    else
      echo "${idx} ${ip} ${port}" >> server.config
    fi
  fi

  idx=$(($idx+1))
done

python3 ${CONFIG_TOOLS_BIN} ./server.config ./server.config.json
mv server.config.json server.config
