#!/bin/bash

# Define processes to monitor
PYROSCOPE_SERVER="pyroscope server"
TARGET_COMMAND="./root/projects/incubator-resilientdb/service/tools/kv/server_tools/start_kv_service_monitoring.sh"

# Kill existing Pyroscope server and target command instances
echo "Terminating existing instances of Pyroscope server and target command..."
pkill -f "$PYROSCOPE_SERVER"
kill -9  kv_service

# Allow time for processes to terminate
sleep 2

# Start Pyroscope server
echo "Starting Pyroscope server..."
nohup pyroscope server > /var/log/pyroscope_server.log 2>&1 &

# Allow Pyroscope server to initialize
sleep 2

# Start the target command
echo "Starting target command..."
$TARGET_COMMAND

