# Vector Indexing SDK for ResilientDB
This directory contains a Python SDK for performing vector indexing and similarity search using ResilientDB as the storage backend.

The primary interface for users is the ```kv_vector.py``` CLI tool, which interacts with the ResilientDB GraphQL service to manage vector embeddings.

## Architecture
- ```kv_vector.py```: The CLI frontend. It sends GraphQL mutations and queries to the proxy.
- ```kv_vector_library.py```: Handles the HTTP requests to the GraphQL endpoint.
### Backend Scripts
- ```vector_add.py```, ```vector_get.py```, ```vector_delete.py```: These scripts reside on the server side (or strictly connected environment) to handle embedding generation (via SentenceTransformers) and HNSW index management.

## Prerequisites
Before using this SDK, please ensure the entire ResilientDB stack is up and running. Specifically, you need:
1. ResilientDB KV Store: The core blockchain storage service must be running. [How to Setup](https://github.com/apache/incubator-resilientdb)
2. GraphQL Server (```ecosystem/graphql```): The backend service handling GraphQL schemas and resolvers. [How to Setup](https://github.com/apache/incubator-resilientdb/tree/master/ecosystem/graphql)
3. GraphQL Application (```ecosystem/graphql/app.py```): The Python web server (Ariadne/Flask) that exposes the GraphQL endpoint. [How to Setup](https://github.com/apache/incubator-resilientdb/tree/master/ecosystem/graphql)
   - Default Endpoint: http://127.0.0.1:8000/graphql
4. In a terminal where the current directory is ecosystem/sdk/vector-indexing, activate the GraphQL virtual environment.

## Installation
Install the required Python dependencies:
```
pip install requests pyyaml numpy hnswlib sentence-transformers
```

## Quick Start: Demo Data
A shell script is provided to quickly populate the database with sample data for testing purposes. This is the fastest way to verify your environment is set up correctly.
1. Make sure you are in the ```ecosystem/sdk/vector-indexing``` directory.
2. Run the demo script:
   ```
   bash demo_add.sh
   ```
   **What this does:** The script iterates through a predefined list of sentences (covering topics like biology, sports, and art) and adds them to the ResilientDB vector index one by one using ```kv_vector.py```.

## Usage (CLI)
The ```kv_vector.py``` script is the main entry point. It allows you to add text (which is automatically vectorized), search for similar text, and manage records via the GraphQL endpoint.

### 1. Adding Data
To add a text string. This will generate an embedding and store it in ResilientDB.
```
python3 kv_vector.py --add "<TEXT>"
```

### 2. Searching
To find the ```k``` most similar strings to your query using HNSW similarity search.
```
# Get the single most similar record (default k=1)
python3 kv_vector.py --get "<SEARCH WORDS>"

# Get the top 3 matches
python3 kv_vector.py --get "<SEARCH WORDS>" --k_matches 3

### 3. Listing All Data
To retrieve all text values currently stored in the index.
```
python3 kv_vector.py --getAll
```

### 4. Deleting Data
To remove a specific value and its embedding from the index.
```
python3 kv_vector.py --delete "<TEXT>"
```

## Configuration
If your GraphQL service is running on a different host or port, you may need to modify the configuration in ```kv_vector_library.py``` or the ```config.yaml``` file depending on your deployment mode.
