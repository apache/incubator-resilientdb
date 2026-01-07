#!/bin/bash
#
# Kill a specific replica or all kv_service processes
# Usage: ./stop_replica.sh [replica_number]
# Example: ./stop_replica.sh 1  (kills only replica 1)
#          ./stop_replica.sh    (kills all replicas)
#

if [ -z "$1" ]; then
  echo "Killing all kv_service processes..."
  killall -9 kv_service
  echo "Done"
else
  REPLICA_NUM=$1
  if ! [[ "$REPLICA_NUM" =~ ^[1-5]$ ]]; then
    echo "Error: Replica number must be between 1 and 5"
    exit 1
  fi
  
  PORT=$((8089 + REPLICA_NUM))
  echo "Killing kv_service on port $PORT (replica $REPLICA_NUM)..."
  
  # Find and kill process on specific port
  PID=$(lsof -i :$PORT -t 2>/dev/null)
  if [ -z "$PID" ]; then
    echo "No process found on port $PORT"
  else
    kill -9 $PID
    echo "Killed PID: $PID"
  fi
fi
