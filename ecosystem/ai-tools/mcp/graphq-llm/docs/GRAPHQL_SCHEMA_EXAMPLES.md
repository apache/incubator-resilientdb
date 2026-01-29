<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

# GraphQL Schema Examples

Based on `app.py` from incubator-resilientdb-graphql repository.

## GraphQL Schema Structure

### Query Type

#### `getTransaction(id: ID!): RetrieveTransaction`

Retrieves a transaction by its ID.

**Example Query:**
```graphql
query {
  getTransaction(id: "abc123") {
    id
    version
    amount
    uri
    type
    publicKey
    signerPublicKey
    operation
    metadata
    asset
  }
}
```

**Response:**
```json
{
  "data": {
    "getTransaction": {
      "id": "abc123",
      "version": "2.0",
      "amount": 100,
      "uri": "uri://...",
      "type": "Ed25519Sha256",
      "publicKey": "public_key_string",
      "signerPublicKey": "signer_public_key",
      "operation": "CREATE",
      "metadata": null,
      "asset": { /* JSON object */ }
    }
  }
}
```

### Mutation Type

#### `postTransaction(data: PrepareAsset!): CommitTransaction`

Creates and commits a new transaction to ResilientDB.

**Input Type: `PrepareAsset`**
```graphql
input PrepareAsset {
  operation: String!
  amount: Int!
  signerPublicKey: String!
  signerPrivateKey: String!
  recipientPublicKey: String!
  asset: JSON!
}
```

**Example Mutation:**
```graphql
mutation {
  postTransaction(
    data: {
      operation: "CREATE"
      amount: 100
      signerPublicKey: "public_key_string"
      signerPrivateKey: "private_key_string"
      recipientPublicKey: "recipient_public_key"
      asset: {
        "data": {
          "name": "My Asset",
          "description": "Asset description"
        }
      }
    }
  ) {
    id
  }
}
```

**Response:**
```json
{
  "data": {
    "postTransaction": {
      "id": "transaction_id_here"
    }
  }
}
```

## Transaction Types

### RetrieveTransaction

Fields returned when retrieving a transaction:

- `id: String` - Transaction ID
- `version: String` - Transaction version
- `amount: Int` - Transaction amount
- `uri: String` - URI condition
- `type: String` - Condition type (e.g., "Ed25519Sha256")
- `publicKey: String` - Public key from output condition
- `signerPublicKey: String` - Public key from input (signer)
- `operation: String` - Operation type ("CREATE" or "TRANSFER")
- `metadata: String` - Optional metadata
- `asset: JSON` - Asset data (JSON object)

### CommitTransaction

Fields returned when committing a transaction:

- `id: String` - Committed transaction ID

## Key-Value Operations

Underlying ResilientDB operations use key-value pairs:

### Key-Value Storage

- Transactions are stored as key-value pairs
- Key: Transaction ID
- Value: JSON object containing transaction data

### Example Key-Value Operations

**Commit Transaction (Key-Value):**
```bash
curl -X POST -d '{"id":"key1","value":"value1"}' \
  http://localhost:18000/v1/transactions/commit
```

**Retrieve Transaction (Key-Value):**
```bash
curl http://localhost:18000/v1/transactions/key1
```

**Get All Transactions:**
```bash
curl http://localhost:18000/v1/transactions
```

## Common Patterns

### Creating an Asset

1. Prepare transaction with operation "CREATE"
2. Specify asset data as JSON
3. Commit transaction
4. Get transaction ID

### Transferring an Asset

1. Prepare transaction with operation "TRANSFER"
2. Specify recipient public key
3. Use existing transaction as input
4. Commit transaction

### Querying Transactions

1. Use `getTransaction` query with transaction ID
2. Retrieve all transaction fields
3. Access asset data from response

## Notes

- All transactions use JSON scalar for flexible asset data
- Private keys are required for mutations but never stored
- Transactions are immutable once committed
- GraphQL provides type safety over raw key-value operations

