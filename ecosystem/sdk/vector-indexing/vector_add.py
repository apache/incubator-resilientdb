"""
Filename: vector_add.py
Description: Save value to ResDB, generate embeddings using SentenceTransformers, and build HNSW index via hnswlib.
"""
import sys
import os
import json
import base64
import numpy as np
from pathlib import Path
from typing import Dict, Any, List

# ML & Search libraries
import hnswlib
from sentence_transformers import SentenceTransformer

# ResDB & Local imports
from resdb_orm.orm import ResDBORM
import hnsw_library

# Global Variables
WORKING_DIR = Path("./").resolve()
db = ResDBORM()
MODEL_NAME = 'all-MiniLM-L6-v2'  # Lightweight and fast model

if __name__ == "__main__":
    # --- SECTION 1: Input Parsing ---
    value_to_add = ''
    for i in range(len(sys.argv)):
        if sys.argv[i] == '--value' and (i + 1 != len(sys.argv)):
            value_to_add = sys.argv[i + 1]
            break

    if value_to_add == '':
        print("Critical Error - requires argument `--value stringToSave`")
        sys.exit()

    # --- SECTION 2: Retrieve/Init Keys & Data ---
    file_saved_directory = Path(WORKING_DIR / "saved_data")
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")
    embedding_keys: Dict[str, Any] = {}
    hnsw_text_entries = []

    if not os.path.exists(file_saved_directory):
        file_saved_directory.mkdir()

    # Load or initialize keys
    if not os.path.exists(file_embedding_keys):
        embedding_keys = {
            "temp_index_txt": "",            # Stores the binary HNSW index (base64)
            "temp_leann_passages_json": "",  # Stores the List[Dict] of text data
            # Unused keys kept for compatibility
            "temp_ids_txt": "",
            "temp_leann_meta_json": "",
            "temp_leann_passages_txt": ""
        }
    else:
        try:
            with open(file_embedding_keys, 'r') as file:
                embedding_keys = json.load(file)
        except Exception:
            pass # Use default empty keys if fail

    # (A) Retrieve existing text entries
    key_passages = embedding_keys.get("temp_leann_passages_json", "")
    try:
        if not key_passages: raise KeyError()
        passages_return = hnsw_library.get_record(key_passages)
        # Expecting data to be List[Dict] -> [{"text": "..."}]
        current_data = passages_return["data"]
        hnsw_text_entries = [item['text'] for item in current_data]

        if value_to_add in hnsw_text_entries:
            print(f"'{value_to_add}' is already saved. Skipping.")
            sys.exit()
        
        hnsw_text_entries.append(value_to_add)
    except Exception:
        # If fetch fails or key doesn't exist, start fresh
        hnsw_text_entries = [value_to_add]
        # Create initial record placeholder
        embedding_keys["temp_leann_passages_json"] = hnsw_library.create_record([])

    # (B) Ensure index key exists
    if not embedding_keys.get("temp_index_txt"):
        embedding_keys["temp_index_txt"] = hnsw_library.create_record('')

    # Save keys locally
    with open(file_embedding_keys, 'w') as file:
        json.dump(embedding_keys, file)

    # --- SECTION 3: Build Index (HNSW + SentenceTransformers) ---
    print("Generating embeddings and building index...")
    
    # 1. Vectorize text
    model = SentenceTransformer(MODEL_NAME)
    embeddings = model.encode(hnsw_text_entries)
    
    # 2. Build HNSW Index
    num_elements = len(embeddings)
    dim = embeddings.shape[1]
    
    # Init HNSW index
    p = hnswlib.Index(space='cosine', dim=dim)
    p.init_index(max_elements=num_elements, ef_construction=200, M=16)
    p.add_items(embeddings, np.arange(num_elements)) # IDs are 0, 1, 2...

    # 3. Save index to temp file
    file_temp_dir = Path(WORKING_DIR / "saved_data/temp")
    if not os.path.exists(file_temp_dir):
        file_temp_dir.mkdir()
        
    index_path = str(file_temp_dir / "hnsw_index.bin")
    p.save_index(index_path)

    # --- SECTION 4: Save to ResDB ---
    print("Saving data to ResDB...")

    # (1) Save Index Binary (Base64 Encoded)
    key_index = embedding_keys["temp_index_txt"]
    try:
        with open(index_path, 'rb') as f:
            content_bytes = f.read()
            # Encode binary to base64 string for JSON transport
            content_b64 = base64.b64encode(content_bytes).decode('utf-8')
            hnsw_library.put_record(key_index, content_b64)
    except Exception as e:
        print(f"Error saving index: {e}")
        sys.exit()

    # (2) Save Text Passages (List of Dicts)
    key_passages = embedding_keys["temp_leann_passages_json"]
    try:
        # Format: [{"text": "val1"}, {"text": "val2"}]
        save_data = [{"text": t} for t in hnsw_text_entries]
        hnsw_library.put_record(key_passages, save_data)
    except Exception as e:
        print(f"Error saving passages: {e}")
        sys.exit()

    # --- Cleanup ---
    if os.path.exists(index_path):
        os.remove(index_path)
    if os.path.exists(file_temp_dir):
        file_temp_dir.rmdir()

    print("Success: Value added and index rebuilt.")