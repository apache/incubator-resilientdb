#!/usr/bin/env bash

set -euo pipefail

SHARD_NUM=4
REPLICAS_PER_SHARD=4
CLIENT_NUM=1
PORT_BASE=10000
SHARD_CONFIG=service/tools/config/shard/shard.config

usage() {
  cat <<'USAGE'
Usage: ./service/tools/kv/server_tools/generate_config_sharded.sh [options]

Options:
  --shards N              Number of shards. Default: 4
  --replicas-per-shard N  Number of replicas per shard. Default: 4
  --clients N             Number of client/proxy nodes to configure. Default: 1
  --port-base N           Base port. Node i uses base + i. Default: 10000
  --shard-config PATH     Output shard config path. Default: service/tools/config/shard/shard.config
  -h, --help              Show this help message.
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
    --port-base)
      [[ $# -ge 2 ]] || { echo "Missing value for --port-base" >&2; exit 1; }
      PORT_BASE="$2"
      shift 2
      ;;
    --shard-config)
      [[ $# -ge 2 ]] || { echo "Missing value for --shard-config" >&2; exit 1; }
      SHARD_CONFIG="$2"
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
is_positive_int "${PORT_BASE}" || {
  echo "--port-base must be an integer >= 1" >&2
  exit 1
}

REPLICA_NUM=$((SHARD_NUM * REPLICAS_PER_SHARD))

echo "Generating sharded 3PC KV configs: shards=${SHARD_NUM} replicas_per_shard=${REPLICAS_PER_SHARD} replicas=${REPLICA_NUM} clients=${CLIENT_NUM} port_base=${PORT_BASE}"

./service/tools/kv/server_tools/generate_config.sh \
  --replicas "${REPLICA_NUM}" \
  --clients "${CLIENT_NUM}" \
  --port-base "${PORT_BASE}"

mkdir -p "$(dirname "${SHARD_CONFIG}")"

python3 - "${SHARD_NUM}" "${REPLICAS_PER_SHARD}" "${CLIENT_NUM}" "${SHARD_CONFIG}" <<'PY'
import json
import sys

shard_num = int(sys.argv[1])
replicas_per_shard = int(sys.argv[2])
client_num = int(sys.argv[3])
shard_config = sys.argv[4]

shards = []
for shard_id in range(shard_num):
    first_replica = shard_id * replicas_per_shard + 1
    replica_ids = list(range(first_replica, first_replica + replicas_per_shard))
    shards.append({
        "shard_id": shard_id,
        "leader_id": replica_ids[0],
        "replica_ids": replica_ids,
    })

replica_num = shard_num * replicas_per_shard
client_ids = list(range(replica_num + 1, replica_num + client_num + 1))

with open(shard_config, "w") as output:
    json.dump({"shards": shards, "client_ids": client_ids}, output, indent=2)
    output.write("\n")
PY

echo "shard config done: ${SHARD_CONFIG}"
