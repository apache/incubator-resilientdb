#!/bin/bash

# Ensure the script is being run from the root directory of the smart-contracts-graphql repository
REPO_NAME="smart-contracts-graphql"
CURRENT_DIR=${PWD##*/}

if [ "$CURRENT_DIR" != "$REPO_NAME" ]; then
  echo "Please run this script from the root directory of the $REPO_NAME repository."
  exit 1
fi

# Clone the incubator-resilientdb repository into $HOME
cd $HOME
git clone https://github.com/apache/incubator-resilientdb.git
cd incubator-resilientdb/

# Run the installation script
./INSTALL.sh

# Start the contract service
./service/tools/contract/service_tools/start_contract_service.sh

# Build the contract tools and kv service tools using Bazel
bazel build service/tools/contract/api_tools/contract_tools
bazel build service/tools/kv/api_tools/kv_service_tools

# Return to $HOME
cd $HOME

# Clone the ResContract repository into $HOME
git clone https://github.com/ResilientEcosystem/ResContract.git
cd ResContract/

# Update and install necessary packages
sudo apt update
sudo apt install -y nodejs npm
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.1/install.sh | bash
source ~/.bashrc
nvm install node

# Install npm globally
sudo npm install -g
npm install commander

# Install latest node
sudo npm install -g n
sudo n latest
hash -r
node --version

# Add Ethereum repository and install solc
sudo add-apt-repository -y ppa:ethereum/ethereum
sudo apt-get update
sudo apt-get install -y solc

echo "All dependencies have been installed successfully."
