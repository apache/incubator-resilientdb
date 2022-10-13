#!/bin/bash

WORK_PATH=$1
WORK_SPACE=${WORK_PATH}/../../

cd "${WORK_PATH}"

IPS=`cat ./deploy/iplist.txt`

KEY_TOOLS_BIN=${WORK_SPACE}/bazel-bin/tools/key_generator_tools
CERT_TOOLS_BIN=${WORK_SPACE}/bazel-bin/tools/certificate_tools
ADMIN_PRIVATE_KEY=${WORK_SPACE}/cert/admin.key.pri
ADMIN_PUBLIC_KEY=${WORK_SPACE}/cert/admin.key.pub
CERT_PATH=deploy/cert/
DEPLOY_PATH=deploy
CLIENT_NUM=1

bazel build //tools:certificate_tools

echo "" > ${DEPLOY_PATH}/client.config
idx=1
tot=0
for ip in ${IPS[@]}
do
	((tot++))
done
echo $tot

for addr in ${IPS};
do
  t=(${addr//:/ })
  ip=${t[0]}
  port=${t[1]}
  echo "ip:",${ip},"port",${port}
  public_key=${CERT_PATH}/node_${idx}.key.pub 

  # create public key
  # create server config
  # create the public key and certificate
  if [ $(($idx+$CLIENT_NUM)) -gt $tot ] ; then
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} client
    echo "${idx} ${ip} ${port}" >> ${DEPLOY_PATH}/client.config
  else
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} replica
  fi

  idx=$(($idx+1))
done
