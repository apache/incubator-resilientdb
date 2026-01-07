#!/bin/bash
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

# Usage: ./start_replica.sh <replica_number>
# Example: ./start_replica.sh 1  (starts node 1 on port 8090)

if [ -z "$1" ]; then
  echo "Usage: $0 <replica_number>"
  echo "Example: $0 1  (starts replica 1)"
  echo "         $0 2  (starts replica 2)"
  echo "         $0 3  (starts replica 3)"
  echo "         $0 4  (starts replica 4)"
  echo "         $0 5  (starts replica 5)"
  exit 1
fi

REPLICA_NUM=$1

# Validate replica number
if ! [[ "$REPLICA_NUM" =~ ^[1-5]$ ]]; then
  echo "Error: Replica number must be between 1 and 5"
  exit 1
fi

SERVER_PATH=./bazel-bin/service/kv/kv_service
SERVER_CONFIG=service/tools/config/server/server.config
WORK_PATH=$PWD
CERT_PATH=${WORK_PATH}/service/tools/data/cert/
PORT=$((8089 + REPLICA_NUM))
LOG_FILE="server$((REPLICA_NUM - 1)).log"

echo "Starting replica $REPLICA_NUM on port $PORT..."
echo "Log file: $LOG_FILE"

# Build if binary doesn't exist
if [ ! -f "$SERVER_PATH" ]; then
  echo "Building kv_service..."
  bazel build //service/kv:kv_service --define enable_leveldb=True
fi

# Start the replica
nohup $SERVER_PATH $SERVER_CONFIG $CERT_PATH/node${REPLICA_NUM}.key.pri $CERT_PATH/cert_${REPLICA_NUM}.cert $PORT > $LOG_FILE &

echo "Replica $REPLICA_NUM started with PID: $!"
