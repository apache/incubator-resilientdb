"""
Filename: vector_delete.py
Description: Remove value from ResDB list and rebuild HNSW index via hnswlib.
"""
import sys
import os
import json
import base64
import numpy as np
from pathlib import Path
from typing import Dict, Any

import hnswlib
from sentence_transformers import SentenceTransformer
from resdb_orm.orm import ResDBORM
import hnsw_library

# Global Variables
WORKING_DIR = Path("./").resolve()
MODEL_NAME = 'all-MiniLM-L6-v2'

if __name__ == "__main__":
    # --- SECTION 1: Input Parsing ---
    value_to_delete = ''
    for i in range(len(sys.argv)):
        if sys.argv[i] == '--value' and (i + 1 != len(sys.argv)):
            value_to_delete = sys.argv[i + 1]
            break

    if not value_to_delete:
        print("Error: Requires argument `--value stringToDelete`")
        sys.exit()

    # --- SECTION 2: Retrieve Data ---
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")
    try:
        with open(file_embedding_keys, 'r') as file:
            embedding_keys = json.load(file)
    except Exception:
        print("Error: keys file missing.")
        sys.exit()

    hnsw_text_entries = []
    try:
        key_passages = embedding_keys["temp_leann_passages_json"]
        ret = hnsw_library.get_record(key_passages)
        current_data = ret["data"]
        hnsw_text_entries = [item['text'] for item in current_data]
    except Exception as e:
        print(f"Error retrieving data: {e}")
        sys.exit()

    # --- SECTION 3: Modify Data ---
    if value_to_delete not in hnsw_text_entries:
        print(f"Warning: '{value_to_delete}' not found. Nothing deleted.")
        sys.exit()
    
    hnsw_text_entries = [t for t in hnsw_text_entries if t != value_to_delete]
    print(f"Removed '{value_to_delete}'. Rebuilding index...")

    if not hnsw_text_entries:
        print("List is now empty. Please add new data to rebuild index.")
        # Consider clearing the remote record here if desired
        sys.exit()

    # --- SECTION 4: Rebuild & Save (Same logic as vector_add) ---
    
    # 1. Vectorize
    model = SentenceTransformer(MODEL_NAME)
    embeddings = model.encode(hnsw_text_entries)

    # 2. Build Index
    num_elements = len(embeddings)
    dim = embeddings.shape[1]
    p = hnswlib.Index(space='cosine', dim=dim)
    p.init_index(max_elements=num_elements, ef_construction=200, M=16)
    p.add_items(embeddings, np.arange(num_elements))

    # 3. Save Index Temp
    file_temp_dir = Path(WORKING_DIR / "saved_data/temp")
    if not os.path.exists(file_temp_dir):
        file_temp_dir.mkdir()
    index_path = str(file_temp_dir / "hnsw_index.bin")
    p.save_index(index_path)

    # 4. Upload to ResDB
    try:
        # Save Index
        key_index = embedding_keys["temp_index_txt"]
        with open(index_path, 'rb') as f:
            content_b64 = base64.b64encode(f.read()).decode('utf-8')
            hnsw_library.put_record(key_index, content_b64)

        # Save Passages
        key_passages = embedding_keys["temp_leann_passages_json"]
        save_data = [{"text": t} for t in hnsw_text_entries]
        hnsw_library.put_record(key_passages, save_data)

        print("Success: Index rebuilt and saved.")
    except Exception as e:
        print(f"Error saving updates: {e}")

    # Cleanup
    if os.path.exists(index_path):
        os.remove(index_path)
    if os.path.exists(file_temp_dir):
        file_temp_dir.rmdir()