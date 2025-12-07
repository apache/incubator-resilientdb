"""
Filename: vector_delete.py
Description: Remove value from ResDB list and rebuild HNSW index via hnswlib.
Refactored to support persistent execution via VectorDeleteManager to avoid model reload overhead.
"""
import sys
import os
import json
import base64
import numpy as np
from pathlib import Path
from typing import Dict, Any, List

import hnswlib
# SentenceTransformer is imported in __main__ or passed in via the class

# ResDB & Local imports
# Ensure this script can find its dependencies
current_dir = Path(__file__).resolve().parent
sys.path.append(str(current_dir))

from resdb_orm.orm import ResDBORM
import hnsw_library

class VectorDeleteManager:
    """
    Manages the deletion of values from the vector index and ResDB.
    Uses a shared persistent model to rebuild the index efficiently.
    """
    def __init__(self, working_dir: Path, model: Any):
        """
        Initialize with working directory and a pre-loaded model.
        """
        self.working_dir = working_dir
        self.model = model
        self.file_saved_directory = self.working_dir / "saved_data"
        self.file_embedding_keys = self.file_saved_directory / "embedding_keys.json"
        
        # Ensure directories exist (though delete implies they should)
        if not os.path.exists(self.file_saved_directory):
            os.makedirs(self.file_saved_directory, exist_ok=True)

    def delete_value(self, value_to_delete: str) -> str:
        """
        Remove a string value from the index.
        1. Retrieve current data.
        2. Filter out the value.
        3. Re-calculate embeddings for remaining data.
        4. Re-build and save the HNSW index.
        """
        if not value_to_delete:
            return "Error: Value to delete is empty."

        # --- Retrieve Keys ---
        if not os.path.exists(self.file_embedding_keys):
            return "Error: keys file missing. No data to delete."

        try:
            with open(self.file_embedding_keys, 'r') as file:
                embedding_keys = json.load(file)
        except Exception:
            return "Error: Failed to load embedding keys."

        # --- Fetch Data ---
        hnsw_text_entries = []
        try:
            key_passages = embedding_keys.get("temp_leann_passages_json")
            if not key_passages:
                return "Error: Passages key not found."
                
            ret = hnsw_library.get_record(key_passages)
            current_data = ret["data"]
            hnsw_text_entries = [item['text'] for item in current_data]
        except Exception as e:
            return f"Error retrieving data: {e}"

        # --- Modify Data ---
        if value_to_delete not in hnsw_text_entries:
            return f"Warning: '{value_to_delete}' not found. Nothing deleted."
        
        hnsw_text_entries = [t for t in hnsw_text_entries if t != value_to_delete]
        
        if not hnsw_text_entries:
            # If list is empty, we might want to clear the record or handle it gracefully
            # For now, we update it to an empty list and assume index is empty
            # Note: hnswlib might complain if we try to build an index with 0 elements.
            try:
                hnsw_library.put_record(key_passages, [])
                # Also potentially clear the index key, but let's just return
                return "Success: Removed value. List is now empty."
            except Exception as e:
                return f"Error clearing data: {e}"

        # --- Rebuild & Save (Similar to add) ---
        try:
            # 1. Vectorize (using resident model)
            embeddings = self.model.encode(hnsw_text_entries)

            # 2. Build Index
            num_elements = len(embeddings)
            dim = embeddings.shape[1]
            p = hnswlib.Index(space='cosine', dim=dim)
            p.init_index(max_elements=num_elements, ef_construction=200, M=16)
            p.add_items(embeddings, np.arange(num_elements))

            # 3. Save Index Temp
            file_temp_dir = self.working_dir / "saved_data/temp"
            if not os.path.exists(file_temp_dir):
                os.makedirs(file_temp_dir, exist_ok=True)
            
            index_path = str(file_temp_dir / "hnsw_index.bin")
            p.save_index(index_path)

            # 4. Upload to ResDB
            # Save Index
            key_index = embedding_keys["temp_index_txt"]
            with open(index_path, 'rb') as f:
                content_b64 = base64.b64encode(f.read()).decode('utf-8')
                hnsw_library.put_record(key_index, content_b64)

            # Save Passages
            save_data = [{"text": t} for t in hnsw_text_entries]
            hnsw_library.put_record(key_passages, save_data)

            # Cleanup
            if os.path.exists(index_path):
                os.remove(index_path)

            return f"Success: Removed '{value_to_delete}'. Index rebuilt."

        except Exception as e:
            return f"Error rebuilding index: {e}"


if __name__ == "__main__":
    # --- CLI Execution Support ---
    from sentence_transformers import SentenceTransformer
    
    WORKING_DIR = Path("./").resolve()
    MODEL_NAME = 'all-MiniLM-L6-v2'

    # Input Parsing
    value_to_delete = ''
    for i in range(len(sys.argv)):
        if sys.argv[i] == '--value' and (i + 1 != len(sys.argv)):
            value_to_delete = sys.argv[i + 1]
            break

    if not value_to_delete:
        print("Error: Requires argument `--value stringToDelete`")
        sys.exit()

    # Load Model (Expensive)
    model = SentenceTransformer(MODEL_NAME)
    
    # Run Manager
    manager = VectorDeleteManager(WORKING_DIR, model)
    result = manager.delete_value(value_to_delete)
    print(result)