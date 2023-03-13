config=$1
output_path=$2

. ${config}

cd ${output_path}

CERT_TOOLS_BIN=${BAZEL_WORKSPACE_PATH}/bazel-bin/tools/certificate_tools
ADMIN_PRIVATE_KEY=${BAZEL_WORKSPACE_PATH}/cert/admin.key.pri
ADMIN_PUBLIC_KEY=${BAZEL_WORKSPACE_PATH}/cert/admin.key.pub
CONFIG_TOOLS_BIN=${BAZEL_WORKSPACE_PATH}/bazel-bin/tools/generate_region_config
CERT_PATH=cert/
USERNAME=ubuntu
BASE_PORT=17000
CLIENT_NUM=1

echo "" > client.config
echo "" > server.config

bazel build //tools:certificate_tools
bazel build //tools:generate_region_config

idx=1
tot=0
for _ in ${iplist[@]};
do
  tot=$(($tot+1))
done

echo "node num:"$tot

for ip in ${iplist[@]};
do
  port=$((${BASE_PORT}+${idx}))
  public_key=${CERT_PATH}/node_${idx}.key.pub 

  # create public key
  # create server config
  # create the public key and certificate
  if [ $(($idx+$CLIENT_NUM)) -gt $tot ] ; then
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} client
    echo "${idx} ${ip} ${port}" >> client.config
  else
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} replica
    echo "${idx} ${ip} ${port}" >> server.config
  fi

  idx=$(($idx+1))
done

python3 ${CONFIG_TOOLS_BIN} ./server.config ./server.config.json
mv server.config.json server.config
