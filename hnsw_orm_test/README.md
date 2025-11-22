# ResilientDB x LEANN Vector Search Integration (Under construction)
This document explains the internal logic and specifications of indexer.py (a resident index construction service) and manage_data.py (a CLI tool for data manipulation), which are core components for keeping data on ResilientDB in a vector-searchable state.

## 1. Indexer Service (indexer.py)
indexer.py monitors the blockchain (ResilientDB) as the "Single Source of Truth" and acts as a resident process to automatically synchronize the local vector search index (HNSW).

### 1.1 Overview and Responsibilities
Polling Monitoring: Periodically fetches all transactions from the database and detects changes.

State Restoration (Log Replay): Replays the append-only transaction log in chronological order to construct the current state of each key in memory.

Vectorization and Index Construction: Vectorizes the latest text data using an Embedding Model, constructs an HNSW graph structure, and saves it to a file.

### 1.2 Main Classes and Functions
SafeResDBORM(ResDBORM)
A wrapper class inheriting from the ResDBORM class of the resdb_orm library, designed to improve network communication stability.

read_all(self): Fetches all data from the /v1/transactions endpoint. It includes timeout settings and exception handling to prevent the process from crashing even if ResilientDB becomes temporarily unresponsive.

main(): The main service loop, which repeats the following steps every POLL_INTERVAL (configuration value, default is 15 seconds).

### 1.3 Processing Flow Details
The index update process is executed according to the following logic:

Change Detection:

Compares the number of transactions fetched in the previous loop (last_tx_count) with the number fetched this time.

The index reconstruction process starts only if current_count > last_tx_count.

Event Extraction and Normalization:

Extracts necessary fields (original_key, operation, text, timestamp) from the fetched raw transaction data.

JSON parse errors or data in invalid formats are skipped.

Chronological Sorting:

Since the arrival order of transactions is not guaranteed in distributed systems, events are sorted in ascending order based on the timestamp within the payload.

Log Replay (State Application):

Applies sorted events sequentially from the beginning to update the active_docs dictionary.

Add / Upsert: Registers the key and text in the dictionary (overwrites if it already exists).

Update: Updates the content only if the key exists in the dictionary. Update events for non-existent keys are ignored (to prevent inconsistency).

Delete: Removes the entry if the key exists in the dictionary.

Index Construction via LeANN:

Creates an index using the LeANN library for the valid documents remaining in active_docs.

Saved Files:

resdb.leann: The vector index body.

id_mapping.json: Metadata linking search result IDs to the original keys (original_key) and text previews.

## 2. Data Manager (manage_data.py)
manage_data.py is an interface that allows users to insert and manipulate data in ResilientDB from the command line. It is not just a simple HTTP client; it possesses pre-check functions to maintain data integrity.

### 2.1 Overview
Operations: Supports adding (add), updating (update), and deleting (delete) data.

Soft Validation: Includes a feature to check if the target key exists in the database before performing modification operations (update/delete) and issues a warning if it does not.

### 2.2 Command Line Usage
```Bash

# Add new data
python3 manage_data.py add <key> <text>

# Update existing data
python3 manage_data.py update <key> <new_text>

# Delete data
python3 manage_data.py delete <key>
```

### 2.3 Internal Logic and Validation Features
SafeResDBORM.read_all() (Retry Logic)
Similar to the class in indexer.py, but this one adds logic to retry up to 3 times (max_retries = 3) in case of network errors.

get_active_keys(db)
Fetches all transactions currently in the database and uses the same Log Replay logic as indexer.py to generate a "list of currently valid keys".

add_event(key, text, op_type)
The core function for transaction generation.

Integrity Check (Soft Validation):

If the operation type is update or delete, it calls get_active_keys() to confirm whether the target key exists.

Warning: If the key is not found, it displays a warning: [WARNING] Key '...' was not found.

Design Intent: Due to the nature of blockchains, there is a lag between writing and reflection (Eventual Consistency). Therefore, the design does not stop on error but warns the user and proceeds with sending the transaction.

Payload Creation:

Creates a JSON object with the following structure:

```
payload = {
    "original_key": key,
    "text": text,
    "timestamp": time.time(),  # Current time for order guarantee
    "operation": op_type,      # "add", "update", "delete"
    "type": "vector_source"
}
```

Transaction Submission: Sends data to ResilientDB via the ORM and displays a part of the transaction ID upon success.

### 3. Summary: Relationship Between the Two Scripts
These two scripts have a relationship close to the Command Query Responsibility Segregation (CQRS) pattern.

Write Side (manage_data.py): Handles data writing (Commands). Instead of directly modifying the database state, it appends operation logs (events).

Read Side (indexer.py): Handles data reading (Queries). It aggregates and processes the written event logs to generate a "Read Model (Vector Index)" optimized for search.

This architecture realizes high-speed vector search functionality while leveraging the append-only ledger characteristics of ResilientDB.
