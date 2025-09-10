#!/bin/bash

# Load the configuration
source ./config/all_machines.conf

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

    # ssh: 安装 sqlite3
    ssh -o StrictHostKeyChecking=no -i "$key" "$USER_NAME@$IP" "
        sudo apt-get update -o Acquire::ForceIPv4=true &&
        sudo apt-get install -y sqlite3
    " &
    SSH_PID=$!

    # scp: 拷贝文件
    scp -o StrictHostKeyChecking=no -i "$key" "$LOCAL_FILE" "$USER_NAME@$IP:$REMOTE_PATH" &
    SCP_PID=$!

    # 输出两个任务的 PID，便于跟踪
    echo "Started ssh task on $IP with PID $SSH_PID"
    echo "Started scp task on $IP with PID $SCP_PID"
done

# Wait for all background processes to finish
wait

echo "All file copy operations completed."
