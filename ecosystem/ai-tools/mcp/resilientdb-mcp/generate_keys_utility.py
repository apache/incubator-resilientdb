#!/usr/bin/env python3
"""Utility script to generate Ed25519 keypairs for ResilientDB.

This is a standalone utility script for manual key generation.
For automated key generation, use the MCP server's generateKeys tool instead.
"""
import sys
import os

# Get the directory where this script is located
script_dir = os.path.dirname(os.path.abspath(__file__))

# Try to find the ResilientDB resdb_driver path
# From ecosystem/mcp/ to ecosystem/graphql/resdb_driver
possible_paths = [
    # Relative path from mcp to graphql/resdb_driver (go up 1 level to ecosystem/)
    os.path.join(script_dir, '../graphql/resdb_driver'),
    # Absolute path (fallback)
    '/Users/rahul/data/workspace/kanagrah/incubator-resilientdb/ecosystem/graphql/resdb_driver',
]

resilientdb_path = None
for path in possible_paths:
    abs_path = os.path.abspath(path)
    if os.path.exists(abs_path):
        sys.path.insert(0, abs_path)
        resilientdb_path = abs_path
        break

if resilientdb_path is None:
    print("Error: Could not find ResilientDB resdb_driver directory.")
    print("Tried the following paths:")
    for path in possible_paths:
        print(f"  - {os.path.abspath(path)}")
    print(f"\nCurrent script location: {script_dir}")
    sys.exit(1)

try:
    from crypto import generate_keypair
except ImportError:
    print("Error: Could not import generate_keypair from ResilientDB crypto module.")
    print(f"Found path: {resilientdb_path}")
    print(f"Please ensure the crypto.py file exists in: {resilientdb_path}")
    sys.exit(1)

# Generate keypairs
signer = generate_keypair()
recipient = generate_keypair()

print("=" * 70)
print("ResilientDB Key Generator")
print("=" * 70)
print()
print("Signer Keypair:")
print(f"  Public Key:  {signer.public_key}")
print(f"  Private Key: {signer.private_key}")
print()
print("Recipient Keypair:")
print(f"  Public Key:  {recipient.public_key}")
print(f"  Private Key: {recipient.private_key}")
print()
print("=" * 70)
print("Ready-to-use curl command:")
print("=" * 70)
print()
print(f"""curl -X POST http://localhost:8000/graphql \\
  -H "Content-Type: application/json" \\
  -d '{{
    "query": "mutation Test($data: PrepareAsset!) {{ postTransaction(data: $data) {{ id }} }}",
    "variables": {{
      "data": {{
        "operation": "CREATE",
        "amount": 100,
        "signerPublicKey": "{signer.public_key}",
        "signerPrivateKey": "{signer.private_key}",
        "recipientPublicKey": "{recipient.public_key}",
        "asset": {{
          "data": {{
            "name": "Test Asset",
            "description": "My first test asset"
          }}
        }}
      }}
    }}
  }}' | python3 -m json.tool""")
print()
print("=" * 70)
print("Copy and paste the curl command above to create a transaction!")
print("=" * 70)
