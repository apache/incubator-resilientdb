#!/bin/bash

# Load the configuration
source ./config/performance.conf

USER_NAME="ubuntu"

# File paths on the remote machine
SOURCE_FILE="~/resilientdb_tpcc.db"
TARGET_FILE="~/tpcc.db"

# Iterate over each IP in the iplist
for IP in "${iplist[@]}"; do
    echo "Creating a copy of $SOURCE_FILE as $TARGET_FILE on $IP..."
    
    # Execute the copy command remotely via SSH
    ssh -o StrictHostKeyChecking=no -i "$key" $USER_NAME@"$IP" "cp $SOURCE_FILE $TARGET_FILE" &
    
    # Capture the process ID for debugging if needed
    echo "Started copy on $IP with PID $!"
done

# Wait for all background processes to finish
wait

echo "All copy operations completed."
