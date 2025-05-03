#!/bin/bash

# Load the configuration
source ./config/kv_performance_server.conf

USER_NAME="ubuntu"

# File to be copied
LOCAL_FILE="../../resilientdb_tpcc.db"
REMOTE_PATH="~/"

# Check if the local file exists
if [ ! -f "$LOCAL_FILE" ]; then
    echo "Error: File $LOCAL_FILE does not exist."
    exit 1
fi

# Iterate over each IP in the iplist and perform the actions concurrently
for IP in "${iplist[@]}"; do
    echo "Installing sqlite3 and copying file to $IP..."

    # Execute commands concurrently: Install sqlite3 and copy the file
    ssh -o StrictHostKeyChecking=no -i "$key" $USER_NAME@"$IP" "
        sudo apt-get update && 
        sudo apt-get install -y sqlite3
    " &

    scp -o StrictHostKeyChecking=no -i "$key" $LOCAL_FILE $USER_NAME@$IP:$REMOTE_PATH &

    # Capture the process ID for debugging if needed
    echo "Started tasks on $IP with PID $!"
done

# Wait for all background processes to finish
wait

echo "All file copy operations completed."
