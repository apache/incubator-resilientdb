#!/usr/bin/env bash

set -euo pipefail

REPLICA_NUM=4
CLIENT_NUM=1
DO_BUILD=1
BAZEL_ARGS=()

usage() {
  cat <<'USAGE'
Usage: ./service/tools/kv/server_tools/start_kv_service_3pc.sh [options] [-- bazel_args...]

Options:
  --replicas N    Number of replica nodes to launch. Default: 4
  --clients N     Number of client/proxy nodes to launch. Default: 1
  --no-build      Skip bazel build and launch the existing binary.
  -h, --help      Show this help message.

Examples:
  ./service/tools/kv/server_tools/start_kv_service_3pc.sh --replicas 32 --clients 1
  ./service/tools/kv/server_tools/start_kv_service_3pc.sh --replicas 4 --clients 1 -- --define enable_leveldb=True
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
    --no-build)
      DO_BUILD=0
      shift
      ;;
    --)
      shift
      BAZEL_ARGS=("$@")
      break
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

killall -9 kv_service kv_service_3pc 2>/dev/null || true

echo "Cleaning old 3PC KV launch logs"
rm -f server[0-9]*.log client.log client[0-9]*.log

SERVER_PATH=./bazel-bin/service/kv/kv_service_3pc
SERVER_CONFIG=service/tools/config/server/server.config
CERT_PATH=$PWD/service/tools/data/cert

if [[ "${DO_BUILD}" -eq 1 ]]; then
  bazel build //service/kv:kv_service_3pc "${BAZEL_ARGS[@]}"
fi

echo "Launching 3PC KV service: replicas=${REPLICA_NUM} clients=${CLIENT_NUM}"

for ((node_id = 1; node_id <= REPLICA_NUM; node_id++)); do
  log_id=$((node_id - 1))
  log_file="server${log_id}.log"
  nohup "${SERVER_PATH}" \
    "${SERVER_CONFIG}" \
    "${CERT_PATH}/node${node_id}.key.pri" \
    "${CERT_PATH}/cert_${node_id}.cert" \
    > "${log_file}" 2>&1 &
  echo "Started replica node ${node_id}, log=${log_file}"
done

for ((client_idx = 0; client_idx < CLIENT_NUM; client_idx++)); do
  node_id=$((REPLICA_NUM + client_idx + 1))
  if [[ "${CLIENT_NUM}" -eq 1 ]]; then
    log_file="client.log"
  else
    log_file="client${client_idx}.log"
  fi
  nohup "${SERVER_PATH}" \
    "${SERVER_CONFIG}" \
    "${CERT_PATH}/node${node_id}.key.pri" \
    "${CERT_PATH}/cert_${node_id}.cert" \
    > "${log_file}" 2>&1 &
  echo "Started client/proxy node ${node_id}, log=${log_file}"
done
