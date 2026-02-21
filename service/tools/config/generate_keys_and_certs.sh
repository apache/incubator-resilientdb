#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

# Script to generate keys and certificates for ResDB nodes
# Usage: generate_keys_and_certs.sh <base_path> <output_cert_path> <admin_key_path> <base_port> <client_num> <ip1> <ip2> ... <ipN>

base_path=$1; shift
output_cert_path=$1; shift
admin_key_path=$1; shift
base_port=$1; shift
client_num=$1; shift

iplist=$@

# Validate inputs
if [ -z "$base_path" ] || [ -z "$output_cert_path" ] || [ -z "$admin_key_path" ] || [ -z "$base_port" ] || [ -z "$client_num" ]; then
    echo "Error: Missing required parameters"
    echo "Usage: $0 <base_path> <output_cert_path> <admin_key_path> <base_port> <client_num> <ip1> <ip2> ... <ipN>"
    exit 1
fi

if [ -z "$iplist" ]; then
    echo "Error: No IP addresses provided"
    echo "Usage: $0 <base_path> <output_cert_path> <admin_key_path> <base_port> <client_num> <ip1> <ip2> ... <ipN>"
    exit 1
fi

echo "Generate keys and certificates"

echo "base path: $base_path"
echo "output cert path: $output_cert_path"
echo "admin_key_path: $admin_key_path"
echo "base port: $base_port"
echo "client num: $client_num"
echo "IP list: $iplist"

ADMIN_PRIVATE_KEY=${admin_key_path}/admin.key.pri
ADMIN_PUBLIC_KEY=${admin_key_path}/admin.key.pub

CERT_TOOLS_BIN=${base_path}/bazel-bin/tools/certificate_tools
KEYGEN_BIN=${base_path}/bazel-bin/tools/key_generator_tools

# Build required tools
echo "Building certificate and key generation tools..."
bazel build //tools:certificate_tools
if [ $? -ne 0 ]; then
    echo "Error: Failed to build certificate_tools"
    exit 1
fi

bazel build //tools:key_generator_tools
if [ $? -ne 0 ]; then
    echo "Error: Failed to build key_generator_tools"
    exit 1
fi

# Count total nodes
idx=1
tot=0
for _ in ${iplist[@]};
do
  tot=$(($tot+1))
done

echo "Total nodes: $tot"
echo "Base port: $base_port"
echo "Client num: $client_num"

# Ensure cert output directory exists
mkdir -p ${output_cert_path}

# Ensure admin key directory exists
mkdir -p ${admin_key_path}

# Check if admin keys exist, if not generate them
if [ ! -f "${ADMIN_PRIVATE_KEY}" ] || [ ! -f "${ADMIN_PUBLIC_KEY}" ]; then
    echo "Admin keys not found, generating admin keypair in ${admin_key_path}..."
    ${KEYGEN_BIN} ${admin_key_path}/admin

    if [ $? -ne 0 ]; then
        echo "Error: Failed to generate admin keypair"
        exit 1
    fi
    echo "Admin keypair generated successfully"
else
    echo "Using existing admin keys from ${admin_key_path}"
fi

# Generate keys and certificates for each node
echo "Generating keys and certificates for nodes..."
for ip in ${iplist[@]};
do
  port=$((${base_port}+${idx}))
  public_key=${output_cert_path}/node${idx}.key.pub 
  
  echo "Processing node $idx: IP=$ip, Port=$port"

  # Generate node key pair
  echo "  Generating key pair for node $idx..."
  ${KEYGEN_BIN} ${output_cert_path}/node${idx}

  if [ $? -ne 0 ]; then
    echo "Error: Failed to generate key pair for node $idx"
    exit 1
  fi

  # Determine if this is a client or replica node
  if [ $(($idx+$client_num)) -gt $tot ]; then
    node_type="client"
    echo "  Generating certificate for client node $idx..."
  else
    node_type="replica"
    echo "  Generating certificate for replica node $idx..."
  fi

  # Generate certificate
  ${CERT_TOOLS_BIN} ${output_cert_path} ${ADMIN_PRIVATE_KEY} ${ADMIN_PUBLIC_KEY} ${public_key} ${idx} ${ip} ${port} ${node_type}

  if [ $? -ne 0 ]; then
    echo "Error: Failed to generate certificate for node $idx"
    exit 1
  fi

  echo "  Completed node $idx"
  idx=$(($idx+1))
done

echo ""
echo "Keys and certificates generation completed successfully"
echo "Output directory: $output_cert_path"
echo "Admin keys directory: $admin_key_path"
echo ""
echo "Generated files:"
echo "  - Admin keys: ${ADMIN_PRIVATE_KEY}, ${ADMIN_PUBLIC_KEY}"
for i in $(seq 1 $tot); do
  echo "  - Node $i keys: ${output_cert_path}/node${i}.key.pri, ${output_cert_path}/node${i}.key.pub"
  echo "  - Node $i certificate: ${output_cert_path}/cert_${i}.cert"
done
