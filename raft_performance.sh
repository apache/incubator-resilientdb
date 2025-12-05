#!/usr/bin/env bash
set -euo pipefail

# Move into the deploy directory
cd ~/incubator-resilientdb/scripts/deploy

# Run the performance script with the config file
./performance_local/raft_performance.sh config/kv_performance_server_local.conf >out_raft.txt 2>&1
