#!/bin/bash

# Ensure the script exits on error
set -e

# Kill all existing pyroscope processes matching the application name pattern
pyroscope_pids=$(ps aux | grep -E 'pyroscope' | grep -v grep | awk '{print $2}')
if [[ -n "$pyroscope_pids" ]]; then
    echo "Killing existing pyroscope processes..."
    echo "$pyroscope_pids" | xargs sudo kill -9
    echo "All existing pyroscope client processes have been terminated."
else
    echo "No matching pyroscope processes found to kill."
fi

# Check if kv_service is running
if ! ps aux | grep "[k]v_service"; then
    echo "No 'kv_service' processes found."
    exit 1
fi

# Start Pyroscope server
echo "Starting Pyroscope server..."
sudo pyroscope server &

# Wait for the Pyroscope server to be ready
echo "Waiting for Pyroscope server to start..."
while ! curl -s http://localhost:4040 > /dev/null; do
    sleep 1
done
echo "Pyroscope server is up and running."

# Get all PIDs of kv_service
pids=$(ps aux | grep "[k]v_service" | awk '{print $2}' | sort -n)

counter=1

for pid in $pids; do
    client_name="cpp_client_$counter"
    echo "Assigning $client_name to process ID $pid"

    # Run the pyroscope command in the background
    sudo pyroscope connect --spy-name ebpfspy --application-name "$client_name" --pid "$pid" &

    # Increment the counter for the next PID
    counter=$((counter + 1))
done

# Run a pyroscope service for the system
echo "Assigning system-level monitoring to Pyroscope..."
sudo pyroscope connect --spy-name ebpfspy --application-name system --pid -1

ps aux | grep -E 'pyroscope connect.* --application-name cpp_client_[0-9]+'

echo "All processes have been assigned client names."
