"""
Filename: vector_add.py
Description: Save value to ResDB, generate embeddings using SentenceTransformers, and build HNSW index via hnswlib.
Refactored to support both CLI execution and import as a module for persistent server usage.
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
# Note: SentenceTransformer is imported inside the block or passed in to avoid overhead if just checking structure,
# but for the class, we assume the model object is passed.

# ResDB & Local imports
# Ensure this script can find its dependencies when run from different contexts
current_dir = Path(__file__).resolve().parent
sys.path.append(str(current_dir))

from resdb_orm.orm import ResDBORM
import hnsw_library

<<<<<<< HEAD
class VectorIndexManager:
    """
    Manages the HNSW index and ResDB storage for vector embeddings.
    Designed to be initialized once with a loaded model to avoid overhead.
    """
    def __init__(self, working_dir: Path, model: Any):
        """
        Initialize the manager.
        Args:
            working_dir: The directory where local keys and temp files are stored.
            model: A loaded SentenceTransformer model instance.
        """
        self.working_dir = working_dir
        self.model = model
        # self.db = ResDBORM()
        
        # Setup directories
        self.file_saved_directory = self.working_dir / "saved_data"
        self.file_embedding_keys = self.file_saved_directory / "embedding_keys.json"
        
        if not os.path.exists(self.file_saved_directory):
            os.makedirs(self.file_saved_directory, exist_ok=True)

    def add_value(self, value_to_add: str) -> str:
        """
        Add a string value to the index.
        1. Checks if already exists.
        2. Updates local list.
        3. Re-calculates embeddings (naive approach for PoC).
        4. Re-builds HNSW index.
        5. Saves everything to ResDB.
        """
        if not value_to_add:
            return "Critical Error - value is empty"

        embedding_keys: Dict[str, Any] = {}
        hnsw_text_entries = []

        # Load or initialize keys
        if not os.path.exists(self.file_embedding_keys):
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
                with open(self.file_embedding_keys, 'r') as file:
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
                return f"'{value_to_add}' is already saved. Skipping."
            
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
        with open(self.file_embedding_keys, 'w') as file:
            json.dump(embedding_keys, file)

        # --- Build Index (HNSW + SentenceTransformers) ---
        # print("Generating embeddings and building index...")
        
        # 1. Vectorize text (using the pre-loaded model)
        embeddings = self.model.encode(hnsw_text_entries)
        
        # 2. Build HNSW Index
        num_elements = len(embeddings)
        dim = embeddings.shape[1]
        
        # Init HNSW index
        p = hnswlib.Index(space='cosine', dim=dim)
        p.init_index(max_elements=num_elements, ef_construction=200, M=16)
        p.add_items(embeddings, np.arange(num_elements)) # IDs are 0, 1, 2...

        # 3. Save index to temp file
        file_temp_dir = self.working_dir / "saved_data/temp"
        if not os.path.exists(file_temp_dir):
            os.makedirs(file_temp_dir, exist_ok=True)
            
        index_path = str(file_temp_dir / "hnsw_index.bin")
        p.save_index(index_path)

        # --- Save to ResDB ---
        # print("Saving data to ResDB...")

        # (1) Save Index Binary (Base64 Encoded)
        key_index = embedding_keys["temp_index_txt"]
        try:
            with open(index_path, 'rb') as f:
                content_bytes = f.read()
                # Encode binary to base64 string for JSON transport
                content_b64 = base64.b64encode(content_bytes).decode('utf-8')
                hnsw_library.put_record(key_index, content_b64)
        except Exception as e:
            return f"Error saving index: {e}"

        # (2) Save Text Passages (List of Dicts)
        key_passages = embedding_keys["temp_leann_passages_json"]
        try:
            # Format: [{"text": "val1"}, {"text": "val2"}]
            save_data = [{"text": t} for t in hnsw_text_entries]
            hnsw_library.put_record(key_passages, save_data)
        except Exception as e:
            return f"Error saving passages: {e}"

        # --- Cleanup ---
        if os.path.exists(index_path):
            os.remove(index_path)
        # We don't remove the dir here to be safe, or we can if empty.

        return "Success: Value added and index rebuilt."
=======
# Global Variables
WORKING_DIR = Path("./").resolve()
MODEL_NAME = 'all-MiniLM-L6-v2'  # Lightweight and fast model
db = ResDBORM()
>>>>>>> da66f0f3efa3b87ac58da0be4006f5199c1eaf47

if __name__ == "__main__":
    # --- CLI Execution Support ---
    # This block allows the script to still be run from the command line if needed
    from sentence_transformers import SentenceTransformer
    
    WORKING_DIR = Path("./").resolve()
    MODEL_NAME = 'all-MiniLM-L6-v2'  # Lightweight and fast model

    # Input Parsing
    value_to_add = ''
    for i in range(len(sys.argv)):
        if sys.argv[i] == '--value' and (i + 1 != len(sys.argv)):
            value_to_add = sys.argv[i + 1]
            break

    if value_to_add == '':
        # TODO: Check up and make sure that this gets sent out to the GQL
        print("Critical Error - requires argument `--value stringToSave`")
        sys.exit()

    # Load Model (Expensive operation)
    model = SentenceTransformer(MODEL_NAME)
    
    # Run Manager
    manager = VectorIndexManager(WORKING_DIR, model)
    result = manager.add_value(value_to_add)
    print(result)