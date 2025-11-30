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
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")
    embedding_keys: Dict[str, Any] = {}

    # Retrieve the keys saving the location of embedding data
    try:
        # We direct this to except instead of a typical if/else to avoid rewriting the same line of code
        if (not os.path.exists(file_embedding_keys)): raise FileNotFoundError()
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
    # TODO: The error handling for all 4 of these blocks isnt well thoguht out
    #   It's correct, but it was just copied from vector_add
    # Most of these aren't looped due to the way they're decoded (literally the "encoding" param)
    #   restructuring this to not loop over these 1-item arrays will make the code a lot more readable
    embedding_data = [
        ("temp.ids.txt", "temp_ids_txt")
    ]
    for pairing in embedding_data:        
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])
        key = embedding_keys[pairing[1]]
        file_content = (hnsw_library.get_record(key))["data"]

        try:
            with open(fileName, 'w', encoding="ascii") as file:
                file.write(file_content)

        # TODO: Consider what this error actually means and handle it correctly
        except Exception as e:
            print(f"An error occurred: {e}")



    embedding_data = [
        ("temp.leann.passages.idx", "temp_leann_passages_txt"),
        ("temp.index", "temp_index_txt")
    ]
    for pairing in embedding_data:        
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])
        key = embedding_keys[pairing[1]]
        file_content = (hnsw_library.get_record(key))["data"]

        try:
            with open(fileName, 'w', encoding="Windows-1252") as file:
                file.write(file_content)

        # TODO: Consider what this error actually means and handle it correctly
        except Exception as e:
            print(f"An error occurred: {e}")


    embedding_data = [
        ("temp.leann.meta.json", "temp_leann_meta_json")
    ]

    for pairing in embedding_data:        
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])
        key = embedding_keys[pairing[1]]
        file_content = (hnsw_library.get_record(key))["data"]

        try:
            with open(fileName, 'w') as file:
                json.dump(file_content, file)

        # TODO: Consider what this error actually means and handle it correctly
        except Exception as e:
            print(f"An error occurred: {e}")


    embedding_data = [
        ("temp.leann.passages.jsonl", "temp_leann_passages_json")
    ]
    for pairing in embedding_data:
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])

        # Since each json object is on a new line (it's a jsonl file), we append instead of overwriting
        # So, we must force the file to delete/recreate to avoid appending over old data
        filePath = Path(fileName)
        if filePath.is_file():
            os.remove(fileName)
        key = embedding_keys[pairing[1]]
        file_content: List[Dict[str, Any]] = (hnsw_library.get_record(key))["data"]

        try:
            with open(fileName, 'a') as file:
                for i, line in enumerate(file_content):
                    json.dump(line, file)
                    if i != (len(file_content) - 1):
                        file.write("\n")

        # TODO: Consider what this error actually means and handle it correctly
        except Exception as e:
            print(f"An error occurred: {e}")

# - - - - - - - - - SECTION 4: Re-Construct the HNSW data structure (leann searcher) - - - - - - - - - >
    # TODO: Rethink the name of this and the input variables
    k_searches = sys.maxsize if return_all else k_matches
    # TODO: Suppress outputs from leann algorithms, they flood the console
    file_temporary_storage = str(WORKING_DIR / "saved_data/temp/temp.leann")
    searcher = LeannSearcher(file_temporary_storage)
    results = searcher.search(search_value, top_k=k_searches)

    # Print results to the console
    for i, line in enumerate(results):
        # TODO: There's probably a faster way to just merge this into the formatting instead of if/else
        if return_all:
            print(f"{i+1}. {line.text}")
        else:
            print(f"{i+1}. {line.text} // (similarity score: {line.score})")

# - - - - - - - - - SECTION 5: Cleanup: Remove temporary files - - - - - - - - - >
    # TODO: Ultimately remove (and at the start of this file, create) the whole temp directory
    # Remove all temporarily created files
    for temp_file_path in ["temp.ids.txt", "temp.index", "temp.leann.passages.idx",
        "temp.leann.meta.json", "temp.leann.passages.jsonl"]:
        fileName = str(WORKING_DIR / "saved_data/temp/" / temp_file_path)

        try:
            os.remove(fileName)
        # TODO: These are non-critical errors, but do this better
        except FileNotFoundError:
            print(f"Error: The file was not found.")
            # TODO: Figure out when this happens (if sudo venv/activate is enough) and warn the user appropriately
        except PermissionError:
            print("Permission error when deleting files in your temp directory")
        except Exception as e:
            print(f"An error occurred: {e}")