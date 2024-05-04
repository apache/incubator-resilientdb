#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
base_path=$1; shift
key_path=$1; shift
output_cert_path=$1; shift
output_path=$1; shift
admin_key_path=$1; shift

iplist=$@

echo "generage certificates"

echo "base path:"$base_path
echo "key path:"$key_path
echo "output cert path:"$output_cert_path
echo "output path:"$output_path
echo "admin_key_path:"$admin_key_path

cd ${output_path}

ADMIN_PRIVATE_KEY=${admin_key_path}/admin.key.pri
ADMIN_PUBLIC_KEY=${admin_key_path}/admin.key.pub

CERT_TOOLS_BIN=${base_path}/bazel-bin/tools/certificate_tools
CONFIG_TOOLS_BIN=${base_path}/bazel-bin/tools/generate_region_config

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
  public_key=${key_path}/node_${idx}.key.pub 

  # create public key
  # create server config
  # create the public key and certificate
  if [ $(($idx+$CLIENT_NUM)) -gt $tot ] ; then
    $CERT_TOOLS_BIN ${output_cert_path} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} client
    echo "${idx} ${ip} ${port}" >> client.config
  else
    $CERT_TOOLS_BIN ${output_cert_path} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} replica
    echo "${idx} ${ip} ${port}" >> server.config
  fi

  idx=$(($idx+1))
done

python3 ${CONFIG_TOOLS_BIN} ./server.config ./server.config.json ../config/template.config
mv server.config.json server.config
