"""
Filename: vector_get.py
Description: Retrieve index/data from ResDB and search using hnswlib.
Refactored to support persistent execution via VectorSearchManager to avoid model reload overhead.
"""
import sys
import os
import json
import base64
import numpy as np
from pathlib import Path
from typing import Dict, Any, List, Tuple, Optional

import hnswlib
# SentenceTransformer is imported in __main__ or passed in via the class

# ResDB & Local imports
# Ensure this script can find its dependencies
current_dir = Path(__file__).resolve().parent
sys.path.append(str(current_dir))

from resdb_orm.orm import ResDBORM
import hnsw_library

class VectorSearchManager:
    """
    Manages retrieval and search operations using a persistent model.
    """
    def __init__(self, working_dir: Path, model: Any):
        """
        Initialize with working directory and a pre-loaded model.
        """
        self.working_dir = working_dir
        self.model = model
        self.file_embedding_keys = self.working_dir / "saved_data/embedding_keys.json"
        self.file_temp_dir = self.working_dir / "saved_data/temp"
        
        if not os.path.exists(self.file_temp_dir):
            os.makedirs(self.file_temp_dir, exist_ok=True)

    def _get_keys(self) -> Dict[str, Any]:
        if not os.path.exists(self.file_embedding_keys):
            raise FileNotFoundError("Embedding keys not found. Add data first.")
        with open(self.file_embedding_keys, 'r') as file:
            return json.load(file)

    def _fetch_data(self) -> Tuple[List[Dict[str, str]], str]:
        """
        Fetches passages and binary index from ResDB (or cache logic could be added here).
        Writes the binary index to a temp file because hnswlib loads from disk.
        Returns: (passages_data, path_to_temp_index_file)
        """
        keys = self._get_keys()
        
        # (A) Fetch Passages (Text Data)
        key_passages = keys.get("temp_leann_passages_json")
        if not key_passages:
            raise KeyError("Passages key missing in local records.")
        
        # In a real scenario, you might cache this instead of fetching from DB every time
        ret_passages = hnsw_library.get_record(key_passages)
        passages_data = ret_passages["data"] # Expecting List[Dict]

        # (B) Fetch Index (Binary)
        key_index = keys.get("temp_index_txt")
        if not key_index:
            raise KeyError("Index key missing in local records.")
        
        ret_index = hnsw_library.get_record(key_index)
        content_b64 = ret_index["data"]
        content_bytes = base64.b64decode(content_b64)
        
        index_path = str(self.file_temp_dir / "hnsw_index_search.bin")
        with open(index_path, 'wb') as f:
            f.write(content_bytes)
            
        return passages_data, index_path

    def get_all_values(self) -> List[str]:
        """Returns all stored text values."""
        try:
            keys = self._get_keys()
            key_passages = keys.get("temp_leann_passages_json")
            if not key_passages: 
                return []
            
            ret = hnsw_library.get_record(key_passages)
            data = ret["data"]
            return [item['text'] for item in data]
        except Exception as e:
            print(f"Error fetching all values: {e}")
            return []

    def search(self, search_value: str, k: int = 1) -> List[Dict[str, Any]]:
        """
        Executes the search using the pre-loaded model.
        Returns a list of dicts: {'text': str, 'score': float}
        """
        index_path = None
        try:
            # 1. Fetch current data & index from DB
            passages_data, index_path = self._fetch_data()
            
            # 2. Embed Query (using resident model)
            query_vector = self.model.encode([search_value])
            
            # 3. Load Index
            dim = query_vector.shape[1]
            num_elements = len(passages_data)
            
            if num_elements == 0:
                return []

            p = hnswlib.Index(space='cosine', dim=dim)
            # Allow slightly more elements to prevent load error if sizes mismatch slightly
            p.load_index(index_path, max_elements=num_elements + 100)

            # 4. Query
            real_k = min(k, num_elements)
            labels, distances = p.knn_query(query_vector, k=real_k)

            # 5. Format Results
            results = []
            for idx, dist in zip(labels[0], distances[0]):
                text = passages_data[idx]['text']
                # Convert cosine distance to similarity score approx (1 - dist)
                score = 1.0 - dist
                results.append({'text': text, 'score': float(score)})
            
            return results

        except Exception as e:
            print(f"Search failed: {e}")
            return []
        finally:
            # Cleanup temp file
            if index_path and os.path.exists(index_path):
                os.remove(index_path)


if __name__ == "__main__":
    # --- CLI Execution Support ---
    from sentence_transformers import SentenceTransformer
    
    WORKING_DIR = Path("./").resolve()
    MODEL_NAME = 'all-MiniLM-L6-v2'

    # Input Parsing
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

    # Load Model (Expensive)
    model = SentenceTransformer(MODEL_NAME)
    manager = VectorSearchManager(WORKING_DIR, model)

    if return_all:
        results = manager.get_all_values()
        print(f"--- All Stored Values ({len(results)}) ---")
        for i, text in enumerate(results):
            print(f"{i+1}. {text}")
    else:
        results = manager.search(search_value, k_matches)
        # print(f"--- Search Results for '{search_value}' ---")
        for i, item in enumerate(results):
            print(f"{i+1}. {item['text']} // (similarity score: {item['score']:.4f})")