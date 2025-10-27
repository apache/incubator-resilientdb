#!/bin/bash
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Exit immediately if a command exits with a non-zero status.
set -e

echo "Starting installation of ResDB-ORM and its prerequisites..."

# Ensure the script is run from the root directory of the ResDB-ORM repository
if [ ! -f "requirements.txt" ]; then
    echo "Error: requirements.txt not found. Please run this script from the root directory of the ResDB-ORM repository."
    exit 1
fi

# Define base directory
BASE_DIR=$(pwd)

# Define the directories where the repositories will be cloned
RESILIENTDB_DIR="$HOME/resilientdb"
RESILIENTDB_GRAPHQL_DIR="$HOME/resilientdb-graphql"

# Function to get the Python 3 version (e.g., 3.10)
get_python_version() {
    PYTHON_VERSION=$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
    echo "$PYTHON_VERSION"
}

# Function to install the appropriate python3.x-venv package
install_python_venv() {
    PYTHON_VERSION=$(get_python_version)
    VENV_PACKAGE="python${PYTHON_VERSION}-venv"
    echo "Installing $VENV_PACKAGE..."
    sudo apt-get update
    sudo apt-get install -y "$VENV_PACKAGE"
}

# Install prerequisites
echo "Installing prerequisites..."

# Clone kv_server (ResilientDB)
echo "Cloning kv_server (ResilientDB)..."
if [ ! -d "$RESILIENTDB_DIR" ]; then
    git clone https://github.com/apache/incubator-resilientdb.git "$RESILIENTDB_DIR"
    echo "ResilientDB cloned into $RESILIENTDB_DIR."
else
    echo "ResilientDB repository already exists at $RESILIENTDB_DIR."
fi

# Set up kv_server
echo "Setting up kv_server..."
cd "$RESILIENTDB_DIR"

echo "Running ResilientDB INSTALL.sh..."
if [ -f "./INSTALL.sh" ]; then
    ./INSTALL.sh
else
    echo "Error: ResilientDB INSTALL.sh not found."
    exit 1
fi

echo "Starting kv_server..."
if [ -f "./service/tools/kv/server_tools/start_kv_service.sh" ]; then
    ./service/tools/kv/server_tools/start_kv_service.sh
else
    echo "Error: start_kv_service.sh not found."
    exit 1
fi

echo "Building kv_service_tools..."
if command -v bazel &> /dev/null; then
    bazel build service/tools/kv/api_tools/kv_service_tools
else
    echo "Error: Bazel is not installed. Please install Bazel and rerun the script."
    exit 1
fi

# Return to ResDB-ORM directory
cd "$BASE_DIR"

# Clone ResilientDB-GraphQL
echo "Cloning ResilientDB-GraphQL..."
if [ ! -d "$RESILIENTDB_GRAPHQL_DIR" ]; then
    git clone https://github.com/apache/incubator-resilientdb-graphql.git "$RESILIENTDB_GRAPHQL_DIR"
    echo "ResilientDB-GraphQL cloned into $RESILIENTDB_GRAPHQL_DIR."
else
    echo "ResilientDB-GraphQL repository already exists at $RESILIENTDB_GRAPHQL_DIR."
fi

# Set up ResilientDB-GraphQL
echo "Setting up ResilientDB-GraphQL..."
cd "$RESILIENTDB_GRAPHQL_DIR"

# Install Python 3 pip if not installed
echo "Checking for python3-pip..."
if ! command -v pip3 &> /dev/null; then
    echo "python3-pip is not installed. Installing..."
    sudo apt-get update
    sudo apt-get install -y python3-pip
else
    echo "python3-pip is already installed."
fi

# Check if ensurepip is available
echo "Checking for ensurepip module..."
if ! python3 -m ensurepip --version &> /dev/null; then
    echo "ensurepip is not available. Installing the appropriate python3-venv package..."
    install_python_venv
else
    echo "ensurepip is available."
fi

# Create and activate virtual environment for ResilientDB-GraphQL
echo "Creating virtual environment for ResilientDB-GraphQL..."

if [ ! -f "venv/bin/activate" ]; then
    # Remove existing incomplete venv directory if it exists
    if [ -d "venv" ]; then
        echo "Incomplete virtual environment detected. Removing..."
        rm -rf venv
    fi
    python3 -m venv venv
    echo "Virtual environment created."
else
    echo "Virtual environment already exists and is complete."
fi

echo "Activating virtual environment..."
source venv/bin/activate

# Install dependencies for ResilientDB-GraphQL
echo "Installing dependencies for ResilientDB-GraphQL..."
pip install --upgrade pip
pip install -r requirements.txt

# Build the ResilientDB-GraphQL server using Bazel
echo "Building ResilientDB-GraphQL server..."
if command -v bazel &> /dev/null; then
    bazel build service/http_server:crow_service_main
else
    echo "Error: Bazel is not installed. Please install Bazel and rerun the script."
    exit 1
fi

bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config & 

echo "ResilientDB-GraphQL server has been built."

# Deactivate virtual environment
deactivate

# Return to ResDB-ORM directory
cd "$BASE_DIR"

# Now proceed to set up ResDB-ORM

# Create and activate a virtual environment for ResDB-ORM
echo "Creating virtual environment for ResDB-ORM..."

if [ ! -f "venv/bin/activate" ]; then
    # Remove existing incomplete venv directory if it exists
    if [ -d "venv" ]; then
        echo "Incomplete virtual environment detected. Removing..."
        rm -rf venv
    fi
    python3 -m venv venv
    echo "Virtual environment for ResDB-ORM created."
else
    echo "Virtual environment already exists and is complete."
fi

echo "Activating virtual environment..."
source venv/bin/activate

# Install dependencies
echo "Installing dependencies for ResDB-ORM..."
pip install --upgrade pip
pip install -r requirements.txt

# Configure config.yaml with the Crow endpoint
CROW_ENDPOINT="http://0.0.0.0:18000"
echo "Using Crow endpoint: $CROW_ENDPOINT"

# Replace <CROW_ENDPOINT> in config.yaml with the actual endpoint
if [ -f "config.yaml" ]; then
    sed -i.bak "s|<CROW_ENDPOINT>|$CROW_ENDPOINT|g" config.yaml
    echo "config.yaml updated with the provided Crow endpoint."
else
    echo "Error: config.yaml file not found."
    exit 1
fi

# Install ResDB-ORM package in editable mode for testing
pip install -e .

# Verify installation
echo "Running test script to verify installation..."
python tests/test.py

echo "Installation and verification completed successfully."

# Deactivate virtual environment
deactivate

echo "Installation script has completed. Please start the ResilientDB-GraphQL server manually as described in the instructions."
