#!/bin/bash
# Wrapper to run the Python scalability experiment script with 2 parameters

# Ensure we are in the project root
BASE_DIR="$(cd "$(dirname "$0")" && pwd)"

# Path to the Python script
PYTHON_SCRIPT="$BASE_DIR/network_delay_experiment.py"

# Check parameters
if [ $# -ne 3 ]; then
    echo "Usage: $0 <protocol> <impacted replicas> <network delay>"
    exit 1
fi

rm -rf result_*

PROTOCOL=$1
IMPACTED=$2
DELAY=$3

# Capture the Python script's output
OUTPUT=$(python3 "$PYTHON_SCRIPT" "$PROTOCOL" "$IMPACTED" "$DELAY")

# Print the output (or use it later in the script)
echo "returned command: $OUTPUT"

# Run the returned string as a command
eval "$OUTPUT"


