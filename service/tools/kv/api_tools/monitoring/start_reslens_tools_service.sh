#!/bin/bash
#
# Startup script for ResLens Flamegraph Analysis Service
#

# Set the directory to the script location
cd "$(dirname "$0")"

# Function to find and activate virtual environment
activate_venv() {
    # Check for common virtual environment locations
    local venv_paths=(
        "venv"
        "env"
        ".venv"
        ".env"
        "../venv"
        "../env"
        "../../venv"
        "../../env"
        "/opt/resilientdb/venv"
        "/opt/resilientdb/env"
    )
    
    for venv_path in "${venv_paths[@]}"; do
        if [[ -d "$venv_path" && -f "$venv_path/bin/activate" ]]; then
            echo "Found virtual environment at: $venv_path"
            source "$venv_path/bin/activate"
            return 0
        fi
    done
    
    # Check if we're already in a virtual environment
    if [[ -n "$VIRTUAL_ENV" ]]; then
        echo "Already in virtual environment: $VIRTUAL_ENV"
        return 0
    fi
    
    echo "No virtual environment found. Using system Python."
    return 1
}

# Try to activate virtual environment
if activate_venv; then
    echo "Using virtual environment: $VIRTUAL_ENV"
else
    echo "Using system Python"
fi

# Check Python version
python_version=$(python3 --version 2>&1)
echo "Python version: $python_version"

# Check if gunicorn is installed
if ! python3 -c "import gunicorn" &> /dev/null; then
    echo "Gunicorn not found. Installing..."
    pip install gunicorn
fi

# Check if Flask is installed
if ! python3 -c "import flask" &> /dev/null; then
    echo "Flask not found. Installing..."
    pip install flask flask-cors
fi

echo "Starting ResLens Flamegraph Analysis Service with Gunicorn..."

# Run with gunicorn
gunicorn \
    --config gunicorn.conf.py \
    --bind 0.0.0.0:8080 \
    --workers 1 \
    --timeout 30 \
    --log-level info \
    reslens_tools_service:app 