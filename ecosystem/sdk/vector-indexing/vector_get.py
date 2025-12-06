"""
Filename: vector_get.py
Description: Retrieve index/data from ResDB and search using hnswlib.
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
    search_value = ""
    k_matches = 1
    return_all = False

    for i in range(len(sys.argv)):
        if sys.argv[i] == '--value' and (i + 1 != len(sys.argv)):
            search_value = sys.argv[i + 1]
        if sys.argv[i] == '--k_matches' and (i + 1 != len(sys.argv)):
            try:
                k_matches = int(sys.argv[i + 1])
            except ValueError:
                k_matches = 1
        if sys.argv[i] == '--show_all':
            return_all = True

    if not search_value and not return_all:
        print("Error: Provide --value 'query' or --show_all")
        sys.exit()

    # --- SECTION 2: Retrieve Keys ---
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")
    try:
        with open(file_embedding_keys, 'r') as file:
            embedding_keys = json.load(file)
    except Exception:
        print("Error: Could not load embedding keys. Add data first.")
        sys.exit()

    # --- SECTION 3: Fetch Data from ResDB ---
    file_temp_dir = Path(WORKING_DIR / "saved_data/temp")
    if not os.path.exists(file_temp_dir):
        file_temp_dir.mkdir()

    # (A) Fetch Passages (Text Data)
    passages_data = []
    try:
        key_passages = embedding_keys["temp_leann_passages_json"]
        ret = hnsw_library.get_record(key_passages)
        passages_data = ret["data"] # Expecting List[Dict]
    except Exception as e:
        print(f"Error retrieving passages: {e}")
        sys.exit()

    if return_all:
        print(f"--- All Stored Values ({len(passages_data)}) ---")
        for i, item in enumerate(passages_data):
            print(f"{i+1}. {item['text']}")
        sys.exit()

    # (B) Fetch Index (Binary)
    index_path = str(file_temp_dir / "hnsw_index.bin")
    try:
        key_index = embedding_keys["temp_index_txt"]
        ret = hnsw_library.get_record(key_index)
        content_b64 = ret["data"]
        content_bytes = base64.b64decode(content_b64)
        with open(index_path, 'wb') as f:
            f.write(content_bytes)
    except Exception as e:
        print(f"Error retrieving index: {e}")
        sys.exit()

    # --- SECTION 4: Search ---
    try:
        # 1. Embed Query
        model = SentenceTransformer(MODEL_NAME)
        query_vector = model.encode([search_value])
        
        # 2. Load Index
        dim = query_vector.shape[1]
        num_elements = len(passages_data)
        
        # Safety check: if index is empty but code ran
        if num_elements == 0:
            print("Index is empty.")
            sys.exit()

        p = hnswlib.Index(space='cosine', dim=dim)
        # allow slightly more elements to prevent load error if sizes mismatch slightly
        p.load_index(index_path, max_elements=num_elements + 100)

        # 3. Query
        real_k = min(k_matches, num_elements)
        labels, distances = p.knn_query(query_vector, k=real_k)

        # 4. Output
        # print(f"--- Search Results for '{search_value}' ---")
        for i, (idx, dist) in enumerate(zip(labels[0], distances[0])):
            text = passages_data[idx]['text']
            # Convert cosine distance to similarity score approx (1 - dist)
            score = 1.0 - dist
            print(f"{i+1}. {text} // (similarity score: {score:.4f})")

    except Exception as e:
        print(f"Search failed: {e}")

    # --- Cleanup ---
    if os.path.exists(index_path):
        os.remove(index_path)
    if os.path.exists(file_temp_dir):
        file_temp_dir.rmdir()