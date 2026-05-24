#!/usr/bin/env bash

set -euo pipefail

SHARD_NUM=4
REPLICAS_PER_SHARD=4
CLIENT_NUM=1
SERVER_CONFIG=service/tools/config/server/server.config
SHARD_CONFIG=service/tools/config/shard/shard.config
DO_BUILD=1
BAZEL_ARGS=()

usage() {
  cat <<'USAGE'
Usage: ./service/tools/kv/server_tools/start_kv_service_sharded_3pc_poe.sh [options] [-- bazel_args...]

Options:
  --shards N              Number of shards. Default: 4
  --replicas-per-shard N  Number of replicas per shard. Default: 4
  --clients N             Number of client/proxy nodes to launch. Default: 1
  --server-config PATH    ResDB server config path. Default: service/tools/config/server/server.config
  --shard-config PATH     Shard config path. Default: service/tools/config/shard/shard.config
  --no-build              Skip bazel build and launch the existing binary.
  -h, --help              Show this help message.

Examples:
  ./service/tools/kv/server_tools/start_kv_service_sharded_3pc_poe.sh --shards 4 --replicas-per-shard 4 --clients 1
  ./service/tools/kv/server_tools/start_kv_service_sharded_3pc_poe.sh --shards 2 --replicas-per-shard 4 --clients 2 -- --define enable_leveldb=True
USAGE
}

is_positive_int() {
  [[ "$1" =~ ^[0-9]+$ ]] && (( "$1" >= 1 ))
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --shards)
      [[ $# -ge 2 ]] || { echo "Missing value for --shards" >&2; exit 1; }
      SHARD_NUM="$2"
      shift 2
      ;;
    --replicas-per-shard)
      [[ $# -ge 2 ]] || { echo "Missing value for --replicas-per-shard" >&2; exit 1; }
      REPLICAS_PER_SHARD="$2"
      shift 2
      ;;
    --clients)
      [[ $# -ge 2 ]] || { echo "Missing value for --clients" >&2; exit 1; }
      CLIENT_NUM="$2"
      shift 2
      ;;
    --server-config)
      [[ $# -ge 2 ]] || { echo "Missing value for --server-config" >&2; exit 1; }
      SERVER_CONFIG="$2"
      shift 2
      ;;
    --shard-config)
      [[ $# -ge 2 ]] || { echo "Missing value for --shard-config" >&2; exit 1; }
      SHARD_CONFIG="$2"
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

is_positive_int "${SHARD_NUM}" || {
  echo "--shards must be an integer >= 1" >&2
  exit 1
}
is_positive_int "${REPLICAS_PER_SHARD}" || {
  echo "--replicas-per-shard must be an integer >= 1" >&2
  exit 1
}
is_positive_int "${CLIENT_NUM}" || {
  echo "--clients must be an integer >= 1" >&2
  exit 1
}

REPLICA_NUM=$((SHARD_NUM * REPLICAS_PER_SHARD))

# Keep the POE launcher independent from the baseline sharded 3PC/PBFT
# launcher, but stop any KV variant so ports and log files are not shared.
killall -9 kv_service kv_service_3pc kv_service_sharded_3pc kv_service_sharded_3pc_poe 2>/dev/null || true

echo "Cleaning old KV launch logs"
rm -f server[0-9]*.log client.log client[0-9]*.log

# This binary uses the same server/shard configs as sharded 3PC/PBFT and only
# changes the consensus manager selected by the service entrypoint.
SERVER_PATH=./bazel-bin/service/kv/kv_service_sharded_3pc_poe
CERT_PATH=$PWD/service/tools/data/cert

if [[ "${DO_BUILD}" -eq 1 ]]; then
  bazel build //service/kv:kv_service_sharded_3pc_poe "${BAZEL_ARGS[@]}"
fi

echo "Launching sharded 3PC/POE KV service: shards=${SHARD_NUM} replicas_per_shard=${REPLICAS_PER_SHARD} replicas=${REPLICA_NUM} clients=${CLIENT_NUM}"

for ((node_id = 1; node_id <= REPLICA_NUM; node_id++)); do
  log_id=$((node_id - 1))
  log_file="server${log_id}.log"
  nohup "${SERVER_PATH}" \
    "${SERVER_CONFIG}" \
    "${SHARD_CONFIG}" \
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
    "${SHARD_CONFIG}" \
    "${CERT_PATH}/node${node_id}.key.pri" \
    "${CERT_PATH}/cert_${node_id}.cert" \
    > "${log_file}" 2>&1 &
  echo "Started client/proxy node ${node_id}, log=${log_file}"
done
