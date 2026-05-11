#!/usr/bin/env bash
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

set -euo pipefail

REPLICA_NUM=4
CLIENT_NUM=1
PORT_BASE=10000

usage() {
  cat <<'USAGE'
Usage: ./service/tools/kv/server_tools/generate_keys_and_certs.sh [options]

Options:
  --replicas N    Number of replica node certificates to generate. Default: 4
  --clients N     Number of client/proxy node certificates to generate. Default: 1
  --port-base N   Base port. Node i uses base + i. Default: 10000
  -h, --help      Show this help message.
USAGE
}

is_positive_int() {
  [[ "$1" =~ ^[0-9]+$ ]] && (( "$1" >= 1 ))
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --replicas)
      [[ $# -ge 2 ]] || { echo "Missing value for --replicas" >&2; exit 1; }
      REPLICA_NUM="$2"
      shift 2
      ;;
    --clients)
      [[ $# -ge 2 ]] || { echo "Missing value for --clients" >&2; exit 1; }
      CLIENT_NUM="$2"
      shift 2
      ;;
    --port-base)
      [[ $# -ge 2 ]] || { echo "Missing value for --port-base" >&2; exit 1; }
      PORT_BASE="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

is_positive_int "${REPLICA_NUM}" || {
  echo "--replicas must be an integer >= 1" >&2
  exit 1
}
is_positive_int "${CLIENT_NUM}" || {
  echo "--clients must be an integer >= 1" >&2
  exit 1
}
is_positive_int "${PORT_BASE}" || {
  echo "--port-base must be an integer >= 1" >&2
  exit 1
}

WORKSPACE=$PWD
CERT_PATH=$PWD/service/tools/data/cert/
TOTAL_NODES=$((REPLICA_NUM + CLIENT_NUM))

mkdir -p "${CERT_PATH}"
echo "Cleaning old generated node keys/certs from ${CERT_PATH}"
rm -f "${CERT_PATH}"/node*.key.pri \
      "${CERT_PATH}"/node*.key.pub \
      "${CERT_PATH}"/cert_*.cert

iplist=()
for ((idx = 1; idx <= TOTAL_NODES; idx++)); do
  iplist+=("127.0.0.1")
done

echo "Generating 3PC KV keys/certs: replicas=${REPLICA_NUM} clients=${CLIENT_NUM} port_base=${PORT_BASE}"

./service/tools/config/generate_keys_and_certs.sh \
  "${WORKSPACE}" \
  "${CERT_PATH}" \
  "${CERT_PATH}" \
  "${PORT_BASE}" \
  "${CLIENT_NUM}" \
  "${iplist[@]}"
