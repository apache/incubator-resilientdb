#!/bin/sh

IPS=`cat ./iplist.txt`
KEY=./ssh-2022-03-24.key
KEY_TOOLS_BIN=../bazel-bin/tools/key_generator_tools
CERT_TOOLS_BIN=../bazel-bin/tools/certificate_tools
ADMIN_PRIVATE_KEY=../cert/admin.key.pri
ADMIN_PUBLIC_KEY=../cert/admin.key.pub
CERT_PATH=cert/
USERNAME=ubuntu
BASE_PORT=37000
CLIENT_NUM=1

echo "" > svr_list.txt
echo "" > cli_list.txt
idx=1
tot=0
for _ in ${IPS};
do
  tot=$(($tot+1))
done
echo $tot
for ip in ${IPS};
do
  port=$((${BASE_PORT}+${idx}))
  public_key=${CERT_PATH}/node_${idx}.key.pub 

  # create public key
  # create server config
  # create the public key and certificate
  if [ $(($idx+$CLIENT_NUM)) -gt $tot ] ; then
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} client
    echo "${idx} ${ip}" >> cli_list.txt
  else
    $CERT_TOOLS_BIN ${CERT_PATH} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} replica
    echo "${idx} ${ip}" >> svr_list.txt
  fi

  idx=$(($idx+1))
done
