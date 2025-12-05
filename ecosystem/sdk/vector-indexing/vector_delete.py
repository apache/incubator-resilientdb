"""
Filename: vector_delete.py
Author(s) / Contributor(s): Steven Shoemaker / Regan Yang, Ritesh Patro, Yoshiki Yamaguchi, Tiching Kao
Date: 2025-Fall
Description: (Indexers project) Run to delete a value from the HNSW index by removing it
             from the ResDB passages list and triggering a full index rebuild.
"""
# Typical Python imports
import sys
import os
import json
from pathlib import Path
from typing import Dict, Any, List
import base64
# ResDB & HNSW imports
from resdb_orm.orm import ResDBORM
import hnsw_library
from leann import LeannBuilder

# Global Variables
WORKING_DIR = Path("./").resolve()
db = ResDBORM()

# --- Utility Function to Rebuild and Save the Index ---
def rebuild_and_save_index(embedding_keys: Dict[str, Any], hnsw_text_entries: List[str]):
    """
    Constructs the HNSW index using the current list of text entries and
    saves the resulting files back to ResDB using the stored keys.

    This function incorporates all necessary corrections for file encoding
    and data structure integrity.
    """
    file_temporary_directory = Path(WORKING_DIR / "saved_data/temp")
    file_temporary_storage = str(WORKING_DIR / "saved_data/temp/temp.leann")

    # Create the temp directory if it doesn't exist
    if not os.path.exists(file_temporary_directory):
        file_temporary_directory.mkdir()

    print("Rebuilding HNSW index with remaining entries (This can take a moment)...")

    # Construct the HNSW Tree (creates the 5 files)
    builder = LeannBuilder(backend_name="hnsw")
    for text in hnsw_text_entries:
        builder.add_text(text)
    builder.build_index(file_temporary_storage)

    # --- SECTION 4: Save the new embeddings (All corrections applied here) ---

    # (2/5) Fix 1: Handle binary files correctly to prevent "ASCII characters" error.
    for pairing in [
        ("temp.leann.passages.idx", "temp_leann_passages_txt"),
        ("temp.index", "temp_index_txt")
        ]:
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])
        key = embedding_keys[pairing[1]]
        try:
            # **CORRECTED LINE: Open in binary read mode ('rb')**
            with open(fileName, 'rb') as file:
                content_bytes = file.read()
                # Decode the bytes to a base64 string for ResDB storage
                content = base64.b64encode(content_bytes).decode('utf-8')
                _ = hnsw_library.put_record(key, content)
        except Exception as e:
            print(f"Critical Save Error (2/5 - {pairing[0]}): {e}")
            sys.exit()

    # (3/5) Create embedding information for the index passages file, which is ascii text data
    fileName = str(WORKING_DIR / "saved_data/temp/temp.ids.txt")
    key = embedding_keys["temp_ids_txt"]
    try:
        with open(fileName, 'r', encoding='ascii') as file:
            content = file.read()
            hnsw_library.put_record(key, content)
    except Exception as e:
        print(f"Critical Save Error (3/5 - temp.ids.txt): {e}")
        sys.exit()

    # (4/5) Create embedding information for the metadata file, which is a single json object
    fileName = str(WORKING_DIR / "saved_data/temp/temp.leann.meta.json")
    key = embedding_keys["temp_leann_meta_json"]
    try:
        with open(fileName, 'r') as file:
            content = json.load(file)
            hnsw_library.put_record(key, content)
    except Exception as e:
        print(f"Critical Save Error (4/5 - temp.leann.meta.json): {e}")
        sys.exit()

    # (5/5) Fix 2: Read the newly generated JSONL file and save it as a List[Dict]
    fileName = str(WORKING_DIR / "saved_data/temp/temp.leann.passages.jsonl")
    key = embedding_keys["temp_leann_passages_json"]
    content = []
    try:
        with open(fileName, 'r') as file:
            # We load each json object line-by-line, saving each as an entry in an array
            for line in file:
                content.append(json.loads(line))
            # The result is a list of dictionaries, correcting the prior corruption issue
            hnsw_library.put_record(key, content)
    except Exception as e:
        print(f"Critical Save Error (5/5 - passages_jsonl): {e}")
        sys.exit()

    print("Index rebuild and save complete.")

    # --- SECTION 5: Cleanup: Remove temporary files ---
    for file_temp_embedding in ["temp.ids.txt", "temp.index", "temp.leann.passages.idx",
        "temp.leann.meta.json", "temp.leann.passages.jsonl"]:
        fileName = str(WORKING_DIR / "saved_data/temp/" / file_temp_embedding)
        try:
            os.remove(fileName)
        except Exception:
            pass

    if os.path.exists(file_temporary_directory):
        file_temporary_directory.rmdir()


# - - - - - - - - - SECTION 1: Init and data cleaning - - - - - - - - - >
if __name__ == "__main__":
    # Input Variable
    value_to_delete = ''

    # Parse the value that the user is requesting to delete
    for i in range (len(sys.argv)):
        if(sys.argv[i] == '--value'  and (i + 1 != len(sys.argv))):
            value_to_delete = sys.argv[i + 1]
            break

    if value_to_delete == '':
        print("Critical Error - the program requires an argument in the form of `--value stringToDelete`")
        sys.exit()

# - - - - - - - - - SECTION 2: Retrieve HNSW data keys and passages list - - - - - - - - - >
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")
    embedding_keys: Dict[str, Any] = {}

    # Retrieve the keys saving the location of embedding data
    try:
        if (not os.path.exists(file_embedding_keys)): raise FileNotFoundError()
        with open(file_embedding_keys, 'r') as file:
            embedding_keys = json.load(file)
    except FileNotFoundError:
        print("Critical Error - The file listing key embeddings does not exist. Cannot delete.")
        sys.exit()
    except Exception as e:
        print(f"Critical Error - {e}")
        sys.exit()

    # Get the current list of text passages from ResDB
    try:
        key = embedding_keys["temp_leann_passages_json"]
        if (key is None or key == ""): raise KeyError()

        passages_return_item = hnsw_library.get_record(key)
        passages_return_data = passages_return_item["data"]

        # Extract just the text from the list of dictionaries
        datapointToText = lambda dataPoint: dataPoint['text']
        current_text_entries = list(map(datapointToText, passages_return_data))
    except Exception as e:
        print(f"Critical Error retrieving passages list: {e}")
        print("Cannot find or access the list of saved values. Terminating...")
        sys.exit()

# - - - - - - - - - SECTION 3: Delete the value and trigger rebuild - - - - - - - - - >
    if value_to_delete not in current_text_entries:
        print(f"Error: '{value_to_delete}' not found in the indexed entries. Nothing deleted.")
        # NOTE: Even if nothing is deleted, this still proceeds to rebuild and save, which
        # is necessary to fix the corruption caused by prior runs of vector_add.py
        # pass
        sys.exit()
    else:
        # Create the new list of entries, excluding the one to delete
        current_text_entries = [text for text in current_text_entries if text != value_to_delete]
        print(f"Value '{value_to_delete}' removed from text entries list.")

    # Trigger the rebuild and save
    rebuild_and_save_index(embedding_keys, current_text_entries)

    print(f"\nSUCCESS: The HNSW index has been rebuilt and saved with correct file encodings.")
    if value_to_delete in current_text_entries:
        print(f"The value '{value_to_delete}' was not in the index, but the system files have been repaired.")