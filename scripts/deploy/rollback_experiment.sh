#!/bin/bash
# Wrapper to run the Python scalability experiment script with 2 parameters

# Ensure we are in the project root
BASE_DIR="$(cd "$(dirname "$0")" && pwd)"

# Path to the Python script
PYTHON_SCRIPT="$BASE_DIR/rollback_experiment.py"

# Check parameters
if [ $# -ne 3 ]; then
    echo "Usage: $0 <protocol> <num_faulty_leader> <timer_length>"
    exit 1
fi

rm -rf result_*

PROTOCOL=$1
NUM_FAULTY=$2
TIMEOUT_LENGTH=$3

# Capture the Python script's output
OUTPUT=$(python3 "$PYTHON_SCRIPT" "$PROTOCOL" "$NUM_FAULTY" "$TIMEOUT_LENGTH")

# Print the output (or use it later in the script)
echo "returned command: $OUTPUT"

# Run the returned string as a command
eval "$OUTPUT"


