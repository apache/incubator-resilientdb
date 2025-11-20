# ResilientDB x LEANN Vector Search Integration
Add something...

## File Structure
- config.py: Common configuration (paths, model selection).
- indexer.py: A background service that watches ResilientDB transactions and builds the vector index.
- populate.py: A script to insert sample data into ResilientDB.
- search.py: A client CLI tool to perform semantic search using the generated index.

## Prerequisites
- ResilientDB KV Service must be running. Ensure you can access http://localhost:18000 (or your configured port).
- Python 3.10
- ResDB-ORM Virtual Environment

## Installation
Once the ResDB-ORM virtual environment is activated, ensure dependencies are installed:

```Bash
(venv) pip install resdb-orm leann 
```

## Usage Guide
To see the system in action, you need to run the scripts in a specific order using two separate terminal windows.

***REMINDER: Activate the venv in BOTH terminals!***

### Terminal 1: Start the Indexer
This process needs to run continuously to monitor the blockchain and update the index.

```Bash
# 1. Activate venv
source ~/ResDB-ORM/venv/bin/activate

# 2. Run Indexer
(venv) python indexer.py
```
***Keep this terminal open!***

### Terminal 2: Insert Data & Search
#### Step 1: Populate Data
Run this script to write sample documents into ResilientDB.
```Bash
# 1. Activate venv
source ~/ResDB-ORM/venv/bin/activate

# 2. Run Populate
(venv) python populate.py
```

#### Step 2: Perform Search
Once the indexer confirms the update, you can search the data.
```Bash
(venv) python search.py
```

## Interaction:
Enter a query like blockchain consensus or vector search. The system will return the most relevant documents along with their ResilientDB Transaction IDs.

## Configuration Notes
- Model: By default, this project uses prajjwal1/bert-tiny (128 dim) to ensure low memory usage and stability. You can change this in config.py.
- Determinism: The indexer forces OMP_NUM_THREADS=1 to guarantee that all replicas build the exact same HNSW graph structure.

## Troubleshooting
- ModuleNotFoundError: If you see this error, you likely forgot to activate the virtual environment. Run source ```~/ResDB-ORM/venv/bin/activate```.
- process Killed / OOM: If your process gets killed, ensure you are not running indexer.py and search.py simultaneously if your memory is limited (< 8GB). Stop the indexer (Ctrl+C) before running the search.
- Connection Error: Ensure ResilientDB is running (```./start_kv_service.sh```).
