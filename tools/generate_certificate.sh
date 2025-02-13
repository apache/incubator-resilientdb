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
bazel build tools/certificate_tools

CERT_PATH=cert/
ADMIN_PRIVATE_KEY_PATH=cert/admin.key.pri
ADMIN_PUBLIC_KEY_PATH=cert/admin.key.pub
NODE_PUBLIC_KEY_PATH=
NODE_ID=
IP=
PORT=

options=$(getopt -l "save_path:,node_pub_key:,node_id:,ip:,port:" -o "" -u -- "$@")
#options=$(getopt -l "version:" -o "" -a -- "$@")

usage() { 
        echo "$0 usage: --node_pub_key : the path of the public key  " 
}

set -- $options

while [ $# -gt 0 ]
do
    case $1 in
    --save_path) CERT_PATH="$2"; shift;;
    --node_pub_key) NODE_PUBLIC_KEY_PATH="$2"; shift ;;
    --node_id) NODE_ID="$2"; shift ;;
    --ip) IP="$2"; shift ;;
    --port) PORT="$2"; shift ;;
    --)
    shift
    break;;
    esac
    shift
done

echo "bazel-bin/tools/certificate_tools $CERT_PATH $ADMIN_PRIVATE_KEY_PATH $ADMIN_PUBLIC_KEY_PATH $NODE_PUBLIC_KEY_PATH $NODE_ID $IP $PORT"
bazel-bin/tools/certificate_tools $CERT_PATH $ADMIN_PRIVATE_KEY_PATH $ADMIN_PUBLIC_KEY_PATH $NODE_PUBLIC_KEY_PATH $NODE_ID $IP $PORT

