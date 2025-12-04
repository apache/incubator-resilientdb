#!/usr/bin/env python3
"""Generate Ed25519 keypairs for ResilientDB GraphQL API using ResilientDB's built-in key generation."""
import sys
import os

# Add ResilientDB path
resilientdb_path = '/Users/rahul/data/workspace/kanagrah/incubator-resilientdb/ecosystem/graphql/resdb_driver'
if os.path.exists(resilientdb_path):
    sys.path.insert(0, resilientdb_path)
else:
    print("Warning: ResilientDB path not found. Trying alternative paths...")
    # Try alternative paths
    alt_paths = [
        os.path.join(os.path.dirname(__file__), '../incubator-resilientdb/ecosystem/graphql/resdb_driver'),
        os.path.join(os.path.dirname(__file__), '../../incubator-resilientdb/ecosystem/graphql/resdb_driver'),
    ]
    for path in alt_paths:
        if os.path.exists(path):
            sys.path.insert(0, path)
            resilientdb_path = path
            break

try:
    from crypto import generate_keypair
except ImportError:
    print("Error: Could not import generate_keypair from ResilientDB crypto module.")
    print(f"Please ensure ResilientDB is installed at: {resilientdb_path}")
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
