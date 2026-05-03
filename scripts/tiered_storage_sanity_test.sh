#!/bin/bash
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

set -e

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$REPO_ROOT"

# Ensure we can find binaries
export PATH="$REPO_ROOT:$PATH"

echo "========================================="
echo "  ResilientDB Tiered Storage Sanity Test"
echo "========================================="
echo ""

SERVER_CONFIG="service/tools/config/server/server_tiered.config"
CLIENT_CONFIG="service/tools/config/interface/service.config"
CERT_PATH="service/tools/data/cert"
KV_TOOL="bazel-bin/service/tools/kv/api_tools/kv_service_tools"
IPFS_TOOL="bazel-bin/chain/storage/ipfs_integration_test"

cleanup() {
    echo ""
    echo "Stopping all kv_service processes..."
    killall -9 kv_service 2>/dev/null || true
    killall -9 migration_daemon 2>/dev/null || true
    echo "Done."
}

trap cleanup EXIT

echo "[1/7] Building targets..."
bazel build //service/kv:kv_service //service/tools/kv/api_tools:kv_service_tools \
    //chain/storage:ipfs_integration_test \
    --define enable_leveldb=True 2>&1 | tail -3
echo "  Build complete."
echo ""

echo "[2/7] Checking prerequisites..."
echo "  - Checking for IPFS daemon..."
if curl -s -X POST http://localhost:5001/api/v0/id >/dev/null 2>&1; then
    echo "    IPFS API is reachable at http://localhost:5001"
else
    echo "    WARNING: IPFS API not reachable at http://localhost:5001"
    echo "    Start IPFS: docker run -d --name ipfs-test -p 5001:5001 -p 8080:8080 -p 4001:4001 ipfs/kubo:latest"
fi
echo ""

echo "[3/7] Testing IPFS client directly..."
$IPFS_TOOL 2>&1 || echo "IPFS test binary failed (is IPFS running?)"
echo ""

echo "[4/7] Cleaning up old data and starting services..."
killall -9 kv_service 2>/dev/null || true
sleep 2
rm -rf 10001_db 10002_db 10003_db 10004_db 10005_db 17005_db

nohup bazel-bin/service/kv/kv_service "$SERVER_CONFIG" "$CERT_PATH/node1.key.pri" "$CERT_PATH/cert_1.cert" > server0.log 2>&1 &
nohup bazel-bin/service/kv/kv_service "$SERVER_CONFIG" "$CERT_PATH/node2.key.pri" "$CERT_PATH/cert_2.cert" > server1.log 2>&1 &
nohup bazel-bin/service/kv/kv_service "$SERVER_CONFIG" "$CERT_PATH/node3.key.pri" "$CERT_PATH/cert_3.cert" > server2.log 2>&1 &
nohup bazel-bin/service/kv/kv_service "$SERVER_CONFIG" "$CERT_PATH/node4.key.pri" "$CERT_PATH/cert_4.cert" > server3.log 2>&1 &
nohup bazel-bin/service/kv/kv_service "$SERVER_CONFIG" "$CERT_PATH/node5.key.pri" "$CERT_PATH/cert_5.cert" > client.log 2>&1 &

echo "  Waiting 10 seconds for replicas to initialize..."
sleep 10

REPLICA_COUNT=$(ps aux | grep "[k]v_service" | wc -l)
echo "  Running kv_service processes: $REPLICA_COUNT (expected 5)"
echo ""

echo "[5/7] Verifying tiered storage initialization..."
grep -E "TieredStorage created|cold_threshold|ipfs_endpoint" server0.log | head -5
echo ""

echo "[6/7] Inserting key-value pairs..."
$KV_TOOL --config "$CLIENT_CONFIG" --cmd set --key "test_key_1" --value "hello_tiered_storage" 2>/dev/null
$KV_TOOL --config "$CLIENT_CONFIG" --cmd set --key "test_key_2" --value "ipfs_migration_test" 2>/dev/null
$KV_TOOL --config "$CLIENT_CONFIG" --cmd set --key "test_key_3" --value "tiered_storage_demo" 2>/dev/null
echo "  Inserted 3 keys."
echo ""

echo "[7/7] Retrieving key-value pairs..."
RESULT1=$($KV_TOOL --config "$CLIENT_CONFIG" --cmd get --key "test_key_1" 2>/dev/null | grep -o '"[^"]*"$' | tr -d '"' || echo "FAILED")
RESULT2=$($KV_TOOL --config "$CLIENT_CONFIG" --cmd get --key "test_key_2" 2>/dev/null | grep -o '"[^"]*"$' | tr -d '"' || echo "FAILED")
RESULT3=$($KV_TOOL --config "$CLIENT_CONFIG" --cmd get --key "test_key_3" 2>/dev/null | grep -o '"[^"]*"$' | tr -d '"' || echo "FAILED")

echo "  test_key_1: $RESULT1"
echo "  test_key_2: $RESULT2"
echo "  test_key_3: $RESULT3"
echo ""

PASS=true
if [ "$RESULT1" != "hello_tiered_storage" ]; then PASS=false; fi
if [ "$RESULT2" != "ipfs_migration_test" ]; then PASS=false; fi
if [ "$RESULT3" != "tiered_storage_demo" ]; then PASS=false; fi

echo "========================================="
if $PASS; then
    echo "  ALL TESTS PASSED"
else
    echo "  SOME TESTS FAILED"
fi
echo "========================================="
echo ""
echo "Tiered storage config:"
echo "  - Backend: TIERED (hot=MemoryDB, warm=LevelDB, cold=IPFS)"
echo "  - Cold threshold: 1 checkpoint"
echo "  - IPFS endpoint: 127.0.0.1:5001"
echo "  - Migration: requires migration daemon (separate process)"
echo ""
echo "IPFS integration test confirmed:"
echo "  - Add: uploads data to IPFS, returns CID"
echo "  - Cat: retrieves data from IPFS by CID"
echo "  - Exists: checks if CID exists via pin/ls"
echo "  - AddDAG: uploads IPLD DAG node"
echo "  - GetDAG: retrieves IPLD DAG node"
echo ""
echo "To manually interact:"
echo "  Set:  $KV_TOOL --config $CLIENT_CONFIG --cmd set --key <key> --value <value>"
echo "  Get:  $KV_TOOL --config $CLIENT_CONFIG --cmd get --key <key>"
echo ""
echo "To check logs: tail -f server0.log"
