import os
import shutil
from pathlib import Path
from leann import LeannBuilder, LeannSearcher
import pprint

# --- 0. Setup KVS (dictionary) and Leann ---

# 1. KVS (dictionary) setup (simulating ResilientDB)
kvs = {}

# 2. Leann index file path
INDEX_PATH = str(Path("./").resolve() / "simple_kvs_leann_default.leann")

# 3. Clean up previous index files (for re-running the demo)
if os.path.exists(f"{INDEX_PATH}.meta.json"):
    print(f"Cleaning up existing index at '{INDEX_PATH}'...")
    try:
        base_name = INDEX_PATH.replace('.leann', '')
        os.remove(f"{base_name}.leann.meta.json")
        os.remove(f"{base_name}.leann.passages.jsonl")
        os.remove(f"{base_name}.leann.passages.idx")
        os.remove(f"{base_name}.index") # HNSW index file
    except FileNotFoundError:
        pass
print("-" * 30)


# --- 1. 'Set' Data to KVS and Build Leann Index ---
print("--- 1. Setting data and building Leann index (default mode) ---")

# Initialize LeannBuilder (default settings, no memory-saving config)
# This will use recompute=True, which requires more RAM during search
builder = LeannBuilder(
    backend_name="hnsw",
    embedding_model="sentence-transformers/all-MiniLM-L6-v2" # Using a lightweight model
)

# Data to store (simulating kv_client.Set(key, value))
data_to_set = [
    ("greeting", "Hello ResilientDB! This is a test."),
    ("doc_001", "Leann is a library for vector search."),
    ("doc_002", "HNSW is a fast algorithm for approximate nearest neighbor search.")
]

for key, value in data_to_set:
    
    # (A) ResilientDB 'Set' operation (store in dictionary)
    print(f"SET: KVS Key = '{key}'")
    kvs[key] = value
    
    # (B) Add to Leann index
    # We pass the KVS key in the metadata 'id'
    builder.add_text(value, metadata={"id": key})

# Build the Leann (HNSW) index
builder.build_index(INDEX_PATH)

print("\nCurrent KVS (dictionary) state:")
pprint.pprint(kvs)
print("Leann (HNSW) index built.")
print("-" * 30)


# --- 2. Run Semantic Search (Leann + HNSW) ---
print("--- 2. Running semantic search (default mode) ---")

try:
    # Use 'with' to safely open the LeannSearcher
    # This will start an internal server (requires more RAM)
    with LeannSearcher(INDEX_PATH) as searcher:
        
        query = "What is a fast vector search algorithm?"
        print(f"Query: '{query}'")

        # (1) Run semantic search on Leann (HNSW)
        # We do NOT pass recompute_embeddings=False, using the default
        results = searcher.search(query, top_k=1)

        if results:
            # (2) Get the 'id' from Leann. In this mode,
            # 'id' should be the original string key ("doc_002").
            found_key = results[0].id 
            
            print(f"\n  -> Leann found key: '{found_key}' (Score: {results[0].score:.4f})")
            
            # (3) Use the key from Leann to 'Get' the full data
            #     from the KVS (dictionary). This is the integration.
            final_value = kvs.get(found_key)
            
            print("  -> Final value retrieved from KVS (dictionary):")
            print(f"    {final_value}")
        else:
            print("  -> No results found.")

except FileNotFoundError:
    print(f"ERROR: Index file not found at {INDEX_PATH}.")
except Exception as e:
    print(f"An error occurred during search: {e}")

print("-" * 30)