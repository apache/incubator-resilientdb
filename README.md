
# resilient-node-cache

**TypeScript library for syncing ResilientDB data via WebSocket and HTTP with seamless reconnection.**

[![License](https://img.shields.io/badge/license-Apache%202-blue)](https://www.apache.org/licenses/LICENSE-2.0)

`resilient-node-cache` is a library for establishing and managing real-time synchronization between ResilientDB and MongoDB using WebSocket and HTTP. It includes automatic reconnection, batching, and concurrency management for high-performance syncing.

## Features

- Real-time sync between ResilientDB and MongoDB
- Handles WebSocket and HTTP requests with auto-reconnection
- Configurable synchronization intervals and batch processing
- Provides MongoDB querying for ResilientDB transaction data

## Installation

To use the library, install it via npm:

```bash
npm install resilient-node-cache
```

## Configuration

### MongoDB Configuration

The library uses MongoDB to store ResilientDB data. Configure it by specifying:

- `uri`: MongoDB connection string
- `dbName`: Database name in MongoDB
- `collectionName`: Collection name in MongoDB where ResilientDB data is stored

### ResilientDB Configuration

For ResilientDB configuration, specify:

- `baseUrl`: The base URL for ResilientDB, e.g., `resilientdb://localhost:18000`
- `httpSecure`: Use HTTPS if set to `true`
- `wsSecure`: Use WSS if set to `true`
- `reconnectInterval`: Reconnection interval in milliseconds (optional)
- `fetchInterval`: Fetch interval in milliseconds for periodic syncs (optional)

## Usage

### 1. Syncing Data from ResilientDB to MongoDB

Create a sync script to initialize and start the data synchronization from ResilientDB to MongoDB.

```javascript
// sync.js

const { WebSocketMongoSync } = require('resilient-node-cache');

const mongoConfig = {
  uri: 'mongodb://localhost:27017',
  dbName: 'myDatabase',
  collectionName: 'myCollection',
};

const resilientDBConfig = {
  baseUrl: 'resilientdb://crow.resilientdb.com',
  httpSecure: true,
  wsSecure: true,
};

const sync = new WebSocketMongoSync(mongoConfig, resilientDBConfig);

sync.on('connected', () => {
  console.log('WebSocket connected.');
});

sync.on('data', (newBlocks) => {
  console.log('Received new blocks:', newBlocks);
});

sync.on('error', (error) => {
  console.error('Error:', error);
});

sync.on('closed', () => {
  console.log('Connection closed.');
});

(async () => {
  try {
    await sync.initialize();
    console.log('Synchronization initialized.');
  } catch (error) {
    console.error('Error during sync initialization:', error);
  }
})();
```

This script will:
- Initialize the connection to MongoDB and ResilientDB
- Continuously sync new blocks received from ResilientDB to MongoDB

### 2. Fetching Transactions by Public Key

After syncing, you can retrieve that data from MongoDB depending on your use case. Here’s a sample script to retrieve all transactions associated with a specified public key:

```javascript
// fetchTransactionsWithPublicKey.js

const { MongoClient } = require('mongodb');

const mongoConfig = {
  uri: 'mongodb://localhost:27017',
  dbName: 'myDatabase',
  collectionName: 'myCollection',
};

// The publicKey for which to fetch transactions
const targetPublicKey = "8LUKr81SmkdDhuBNAHfH9C8G5m6Cye2mpUggVu61USbD";

(async () => {
  const client = new MongoClient(mongoConfig.uri);

  try {
    await client.connect();
    const db = client.db(mongoConfig.dbName);
    const collection = db.collection(mongoConfig.collectionName);

    console.log('Connected to MongoDB for fetching transactions.');

    // Create an index on transactions.value.inputs.owners_before for optimized querying
    const indexName = await collection.createIndex({ "transactions.value.inputs.owners_before": 1 });
    console.log(`Index created: ${indexName} on transactions.value.inputs.owners_before`);

    // Define aggregation pipeline to fetch all transactions for the specified publicKey in owners_before
    const pipeline = [
      { $unwind: "$transactions" },
      { $unwind: "$transactions.value.inputs" },
      { 
        $match: { 
          "transactions.value.inputs.owners_before": targetPublicKey 
        }
      },
      { $sort: { "transactions.value.asset.data.timestamp": -1 } },
      { $project: { transaction: "$transactions", _id: 0 } }
    ];

    const cursor = collection.aggregate(pipeline);
    const transactions = await cursor.toArray();

    if (transactions.length > 0) {
      console.log('Transactions with the specified publicKey in owners_before:', 
                  JSON.stringify(transactions, null, 2));
    } else {
      console.log(`No transactions found for publicKey: ${targetPublicKey}`);
    }
  } catch (error) {
    console.error('Error fetching transactions:', error);
  } finally {
    await client.close();
  }
})();
```

### API Documentation

#### Class `WebSocketMongoSync`

- **constructor(mongoConfig: MongoConfig, resilientDBConfig: ResilientDBConfig)**:
  - Initializes the sync object with MongoDB and ResilientDB configurations.

- **initialize()**: Connects to MongoDB, fetches initial blocks, starts periodic fetching, and opens the WebSocket connection to ResilientDB.

- **close()**: Closes the MongoDB and WebSocket connections, stopping the periodic fetching.

### MongoDB Collection Structure

Each document in the MongoDB collection corresponds to a ResilientDB block, containing:
- **id**: Block identifier
- **createdAt**: Timestamp for block creation
- **transactions**: Array of transactions in the block, with details for each transaction’s inputs, outputs, and asset metadata

### License

This library is licensed under the [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

---

Enjoy using `resilient-node-cache` to seamlessly synchronize your ResilientDB data to MongoDB with real-time updates and efficient querying!
