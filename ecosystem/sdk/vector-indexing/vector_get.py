"""
Filename: vector_get.py
Author(s) / Contrubtor(s): Steven Shoemaker / Regan Yang, Ritesh Patro, Yoshiki Yamaguchi, Tiching Kao
Date: 2025-Fall
Description: (Indexers project) Run to search ResDB for the embeddings k-closest to input string
"""
# Typical Python imports
import sys
import os
import json
from pathlib import Path
from typing import Dict, List, Any
import base64
# ResDB & HNSW imports
from resdb_orm.orm import ResDBORM
import hnsw_library
from leann import LeannSearcher

# Global Variables
WORKING_DIR = Path("./").resolve()
db = ResDBORM()

# - - - - - - - - - SECTION 1: Init and data cleaning - - - - - - - - - >
if __name__ == "__main__":
    # Input Variables
    search_value = ""
    k_matches = 0
    return_all = False

    # Parse the values that the user is requesting to add
    for i in range (len(sys.argv)):
        if(sys.argv[i] == '--value'  and (i + 1 != len(sys.argv))):
            search_value = sys.argv[i + 1]

        if(sys.argv[i] == '--k_matches' and (i + 1 != len(sys.argv))):
            # Ensure that k_matches is in the form of a nondecimal number
            try:
                k_matches = int(sys.argv[i + 1])
            except ValueError:
                print("Invalid input - The input to `--k_matches` must be an integer number")
                sys.exit()

        if(sys.argv[i] == '--show_all'):
            return_all = True

    if not return_all:
        # If the user doesn't request to return everything OR search on something specific, error out
        if not search_value:
            print('Invalid input - please use one of the two following combinations of flags:')
            print('(1) Use flag `--value STRING` to find most similar terms to STRING. In addition, use')
            print('     flag `--k_matches ###` to search for the k-closest strings. Leave blank to only find one')
            print('(2) Use flag `--show_all` with no arguements to list ALL values that correlate with a vector embedding')
            print('     this will override all other flags used')
            sys.exit()

        # If the user is searching on a specific string, ensure that the requested number of matches is a whole number
        if k_matches <= 0:
            print('No or invalid arguement provided for --k_matches. Defaulting to finding one single most similar value')
            k_matches = 1
        
# - - - - - - - - - SECTION 2: Retrieve keys to HNSW data - - - - - - - - - >
    file_saved_directory = Path(WORKING_DIR / "saved_data")
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")
    file_embedding_keys_path = Path(WORKING_DIR / "saved_data/embedding_keys.json")
    embedding_keys: Dict[str, Any] = {}

    # Retrieve the keys saving the location of embedding data
    try:
        # We direct this to except instead of a typical if/else to avoid rewriting the same line of code
        if (not os.path.exists(file_saved_directory)): raise FileNotFoundError()
        if (not file_embedding_keys_path.is_file()): raise FileNotFoundError()
        with open(file_embedding_keys, 'r') as file:
            embedding_keys = json.load(file)
    except FileNotFoundError:
        print("Critical Error - The file listing key embeddings does not exist. Please add a vector value before trying to retrieve similar values")
        print("Terminating...")
        os.exit()
    except Exception as e:
        print(f"Critical Error - {e}")
        print("There is no protocol for handling this error, but it is known it will prevent retrieval of embedding data. Terminating...")
        os.exit()
                
# - - - - - - - - - SECTION 3: Save the embedding data to temporary files - - - - - - - - - >
    file_temporary_directory = Path(WORKING_DIR / "saved_data/temp")

    # Create the temp directory if it doesn't exist
    if not os.path.exists(file_temporary_directory):
        file_temporary_directory.mkdir()

    # Embedding information using this library is split across 5 files. The next chunk of code retrieves
    #   each file from ResDB, temporarily saving it

    # (2/5) Save embedding information for the untyped files, which are raw byte data
    embedding_data = [
        ("temp.leann.passages.idx", "temp_leann_passages_txt"),
        ("temp.index", "temp_index_txt")
    ]
    for pairing in embedding_data:        
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])
        key = embedding_keys[pairing[1]]
        file_return_item = hnsw_library.get_record(key)
        file_return_data = file_return_item["data"]
        try:
            with open(fileName, 'wb') as file:
                binary_content = base64.b64decode(file_return_data)
                file.write(binary_content)
        except Exception as e:
            print(f"Unsuccessful ResDB retrieval for untyped file: {e}")
            print("Critical Error - the above error indicates that a file used for vector embeddings is improperly saved")
            print("This likely has ruined the entire embedding system. If you face the same error, delete all your saved")
            print("data by deleting `vector-indexing/saved_data` and start fresh.")
            print("Terminating...")
            sys.exit()
    # (3/5) Save embedding information for the ID text file, which is ascii data       
    fileName = str(WORKING_DIR / "saved_data/temp/temp.ids.txt")
    key = embedding_keys["temp_ids_txt"]
    file_return_item = hnsw_library.get_record(key)
    file_return_data = file_return_item["data"]
    try:
        with open(fileName, 'w', encoding="ascii") as file:
            file.write(file_return_data)
    except Exception as e:
        print(f"Unsuccessful ResDB retrieval for text file: {e}")
        print("Critical Error - the above error indicates that a file used for vector embeddings is improperly saved")
        print("This likely has ruined the entire embedding system. If you face the same error, delete all your saved")
        print("data by deleting `vector-indexing/saved_data` and start fresh.")
        print("Terminating...")
        sys.exit()
    # (4/5) Save embedding information for the json file
    fileName = str(WORKING_DIR / "saved_data/temp/temp.leann.meta.json")
    key = embedding_keys["temp_leann_meta_json"]
    file_return_item = hnsw_library.get_record(key)
    file_return_data = file_return_item["data"]
    try:
        with open(fileName, 'w') as file:
            json.dump(file_return_data, file)
    except Exception as e:
        print(f"Unsuccessful ResDB retrieval for json file: {e}")
        print("Critical Error - the above error indicates that a file used for vector embeddings is improperly saved")
        print("This likely has ruined the entire embedding system. If you face the same error, delete all your saved")
        print("data by deleting `vector-indexing/saved_data` and start fresh.")
        print("Terminating...")
        sys.exit()
    # (5/5) Save embedding information for the jsonLine file
    fileName = str(WORKING_DIR / "saved_data/temp/temp.leann.passages.jsonl")

    # Since each json object is on a new line (it's a jsonl file), we append instead of overwriting
    # So we must force the file to delete/recreate to avoid appending over old data
    filePath = Path(fileName)
    if filePath.is_file():
        os.remove(fileName)
    key = embedding_keys["temp_leann_passages_json"]
    file_return_item = hnsw_library.get_record(key)
    file_return_data: List[Dict[str, Any]] = file_return_item["data"]

    # Delimit each json object with lines, instead of just as entires in a list
    try:
        with open(fileName, 'a') as file:
            for i, line in enumerate(file_return_data):
                json.dump(line, file)
                if i != (len(file_return_data) - 1):
                    file.write("\n")
    except Exception as e:
        print(f"Unsuccessful ResDB retrieval for jsonLine file: {e}")
        print("Critical Error - the above error indicates that a file used for vector embeddings is improperly saved")
        print("This likely has ruined the entire embedding system. If you face the same error, delete all your saved")
        print("data by deleting `vector-indexing/saved_data` and start fresh.")
        print("Terminating...")
        sys.exit()

# - - - - - - - - - SECTION 4: Re-Construct the HNSW data structure (leann searcher) - - - - - - - - - >
    total_searches = sys.maxsize if return_all else k_matches

    # Leann is extremely noisy, prevent standard output to the console while it runs
    # sys.stdout = os.devnull # TODO

    file_temporary_storage = str(WORKING_DIR / "saved_data/temp/temp.leann")
    searcher = LeannSearcher(file_temporary_storage)
    results = searcher.search(search_value, top_k=total_searches)

    # Restore standard output to the console
    # sys.stdout = sys.__stdout__ # TODO

    # Print results to the console
    for i, line in enumerate(results):
        if return_all:
            print(f"{i+1}. {line.text}")
        else:
            print(f"{i+1}. {line.text} // (similarity score: {line.score})")

# - - - - - - - - - SECTION 5: Cleanup: Remove temporary files - - - - - - - - - >
    # Remove all temporary files created during HNSW Tree Search
    for temp_file_path in ["temp.ids.txt", "temp.index", "temp.leann.passages.idx",
        "temp.leann.meta.json", "temp.leann.passages.jsonl"]:
        fileName = str(WORKING_DIR / "saved_data/temp/" / temp_file_path)
        try:
            os.remove(fileName)
        except Exception as e:
            print(f"Error - A problem occurred while deleting temporary data: {e}")
            print("This is non-critical. It is reccomended you delete the folder `vector-indexing/saved_data/temp` to save space")

    # Remove the whole temp directory
    if os.path.exists(file_temporary_directory):
        file_temporary_directory.rmdir()