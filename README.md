
# resilient-python-cache

**Python library for syncing ResilientDB data via WebSocket and HTTP with seamless reconnection.**

[![License](https://img.shields.io/badge/license-Apache%202-blue)](https://www.apache.org/licenses/LICENSE-2.0)

`resilient-python-cache` is a library for establishing and managing real-time synchronization between ResilientDB and MongoDB using WebSocket and HTTP. It includes automatic reconnection, batching, and concurrency management for high-performance syncing.

## Features

- Real-time sync between ResilientDB and MongoDB
- Handles WebSocket and HTTP requests with auto-reconnection
- Configurable synchronization intervals and batch processing
- Provides MongoDB querying for ResilientDB transaction data

## Installation

To use the library, install it via pip:

```bash
pip install resilient-python-cache
```

## Configuration

### MongoDB Configuration

The library uses MongoDB to store ResilientDB data. Configure it by specifying:

- `uri`: MongoDB connection string
- `db_name`: Database name in MongoDB
- `collection_name`: Collection name in MongoDB where ResilientDB data is stored

### ResilientDB Configuration

For ResilientDB configuration, specify:

- `base_url`: The base URL for ResilientDB, e.g., `resilientdb://localhost:18000`
- `http_secure`: Use HTTPS if set to `true`
- `ws_secure`: Use WSS if set to `true`
- `reconnect_interval`: Reconnection interval in milliseconds (optional)
- `fetch_interval`: Fetch interval in milliseconds for periodic syncs (optional)

## Usage

### 1. Syncing Data from ResilientDB to MongoDB

Create a sync script to initialize and start the data synchronization from ResilientDB to MongoDB.

```python
# sync.py

import asyncio
from resilient_python_cache import ResilientPythonCache, MongoConfig, ResilientDBConfig

async def main():
    mongo_config = MongoConfig(
        uri="mongodb://localhost:27017",
        db_name="myDatabase",
        collection_name="myCollection"
    )

    resilient_db_config = ResilientDBConfig(
        base_url="resilientdb://crow.resilientdb.com",
        http_secure=True,
        ws_secure=True
    )

    cache = ResilientPythonCache(mongo_config, resilient_db_config)

    cache.on("connected", lambda: print("WebSocket connected."))
    cache.on("data", lambda new_blocks: print("Received new blocks:", new_blocks))
    cache.on("error", lambda error: print("Error:", error))
    cache.on("closed", lambda: print("Connection closed."))

    try:
        await cache.initialize()
        print("Synchronization initialized.")

        try:
            await asyncio.Future()  # Run indefinitely
        except asyncio.CancelledError:
            pass

    except Exception as error:
        print("Error during sync initialization:", error)
    finally:
        await cache.close()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Interrupted by user")
```

This script will:
- Initialize the connection to MongoDB and ResilientDB
- Continuously sync new blocks received from ResilientDB to MongoDB

### API Documentation

#### Class `ResilientPythonCache`

- **constructor(mongo_config: MongoConfig, resilient_db_config: ResilientDBConfig)**:
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

Enjoy using `resilient-python-cache` to seamlessly synchronize your ResilientDB data to MongoDB with real-time updates and efficient querying!