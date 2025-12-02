"""
Filename: vector_add.py
Author(s) / Contrubtor(s): Steven Shoemaker / Regan Yang, Ritesh Patro, Yoshiki Yamaguchi, Tiching Kao
Date: 2025-Fall
Description: (Indexers project) Run to save value to ResDB and generate a vector embedding for it
"""
# Typical Python imports
import sys
import os
import json
from pathlib import Path
from typing import Dict, Any
import base64
# ResDB & HNSW imports
from resdb_orm.orm import ResDBORM
import hnsw_library
from leann import LeannBuilder

# Global Variables
WORKING_DIR = Path("./").resolve()
db = ResDBORM()

# - - - - - - - - - SECTION 1: Init and data cleaning - - - - - - - - - >
# This entire file is only ever intended to run from a CLI
if __name__ == "__main__":
    # Input Variable
    value_to_add = ''

    # Parse the value that the user is requesting to add
    for i in range (len(sys.argv)):
        # TODO: Consider if you need to parse whitespace around `--value`
        if(sys.argv[i] == '--value'  and (i + 1 != len(sys.argv))):
            value_to_add = sys.argv[i + 1]
            break
    
    if value_to_add == '':
        print("Critical Error - the program requires an arguement in the form of `--value stringToSave`")
        sys.exit()

# - - - - - - - - - SECTION 2: Retrieve HNSW data or create if it doesnt exist - - - - - - - - - >
    embedding_keys: Dict[str, Any] = {}
    hnsw_text_entries = []
    file_saved_directory = Path(WORKING_DIR / "saved_data")
    file_embedding_keys = str(WORKING_DIR / "saved_data/embedding_keys.json")

    # Create the saved_data directory if it doesn't exist
    if not os.path.exists(file_saved_directory):
        file_saved_directory.mkdir()

    # Create the file storing embedding keys if it doesn't exist
    if not os.path.exists(file_embedding_keys):
        embedding_keys = {
            "temp_ids_txt": "",
            "temp_index_txt": "",
            "temp_leann_meta_json": "",
            "temp_leann_passages_txt": "",
            "temp_leann_passages_json": ""
        }
    # If the user does have some form of prior saved data, it should have a list of keys, even if invalid
    else:
        try:
            with open(file_embedding_keys, 'r') as file:
                embedding_keys = json.load(file)
        except FileNotFoundError:
            print("The file storing saved keys could not be found.")
            print("This means that the requested vector to save will be the first to recieve an embedding")
            print("If this is your first time adding an embedding to this database, this is the intended behavior")
        except Exception as e:
            print(f"Unexpected Error - {e}")
            print("The program can continue running, but it will treat this values as the first generated vector embedding")

    # Embedding information is stored in ResDB. The next chunk of code ensures that a place to save this information
    #   exists - either by retrieving it or creating it. There are 5 total files used to store vector data

    # (1/5) Create embedding information for the json passages file, which is stored in ResDB as string array
    key = embedding_keys["temp_leann_passages_json"]
    try:
        if (key is None or key == ""): raise KeyError()

        passages_return_item = hnsw_library.get_record(key)
        passages_return_data = passages_return_item["data"]
        datapointToText = lambda dataPoint: dataPoint['text']
        hnsw_text_entries = list(map(datapointToText, passages_return_data))

        # This file also contains the saved VALUES, check to make sure we aren't re-saving the same data
        if value_to_add in hnsw_text_entries:
            print(f"{value_to_add} is already saved with an embedding in the ResDB database")
            print("Duplicate embeddings yield the same result, this value will not be saved. Terminating...")
            sys.exit()

        hnsw_text_entries.append(value_to_add)
    except Exception:
        hnsw_text_entries = [value_to_add]
        embedding_keys["temp_leann_passages_json"] = hnsw_library.create_record([])
    # (4/5) Create embedding information for text files, which are stored in ResDB as string
    for field in ["temp_ids_txt", "temp_index_txt", "temp_leann_passages_txt"]:
        key = embedding_keys[field]
        try:
            # We direct this to except instead of a typical if/else to avoid rewriting the same line of code
            if (key is None or key == ""): raise KeyError()
            _ = hnsw_library.get_record(key)
        except Exception:
            embedding_keys[field] = hnsw_library.create_record('')
    # (5/5) Create embedding information for the json metadata file, which is stored in ResDB as a Dict
    key = embedding_keys["temp_leann_meta_json"]
    try:
        if (key is None or key == ""): raise KeyError()
        _ = hnsw_library.get_record(key)
    except Exception:
        embedding_keys["temp_leann_meta_json"] = hnsw_library.create_record({})

    # Save the embedding keys to a local file
    try:
        with open(file_embedding_keys, 'w') as file:
            json.dump(embedding_keys, file)
    except Exception as e:
        print(f"Unsuccessful write: {e}")
        print("Critical Error - the above error prevents the program from saving locally the keys necessary to track embedding data")
        print('This prevents the program from using these embeddings in the future. Consequently, terminating...')
        sys.exit()

# - - - - - - - - - SECTION 3: Construct the HNSW data structure (leann builder) - - - - - - - - - >
    file_temporary_directory = Path(WORKING_DIR / "saved_data/temp")
    file_temporary_storage = str(WORKING_DIR / "saved_data/temp/temp.leann")

    # Create the temp directory if it doesn't exist
    if not os.path.exists(file_temporary_directory):
        file_temporary_directory.mkdir()

    # Leann is extremely noisy, prevent standard output to the console while it runs
    # sys.stdout = os.devnull # TODO

    # Construct the HNSW Tree (creates the 5 files referenced below, saved to a temporary folder)
    builder = LeannBuilder(backend_name="hnsw")
    for text in hnsw_text_entries:
        builder.add_text(text)
    builder.build_index(file_temporary_storage)

    # Restore standard output to the console
    # sys.stdout = sys.__stdout__ # TODO

# - - - - - - - - - SECTION 4: Save the new embeddings - - - - - - - - - >
    # Embedding information using this library is split across 5 files. The next chunk of code saves each of
    #   these files as a kv store value in ResDB, storing text data as a string, and JSON data as a Dict or Dict[]

    # (2/5) Create embedding information for the txt passages file, which are raw byte data
    for pairing in [
        ("temp.leann.passages.idx", "temp_leann_passages_txt"),
        ("temp.index", "temp_index_txt")
        ]:
        fileName = str(WORKING_DIR / "saved_data/temp/" / pairing[0])
        key = embedding_keys[pairing[1]]
        try:
            with open(fileName, 'rb') as file:
                binary_content = file.read()
                content = base64.b64encode(binary_content).decode('utf-8')
                _ = hnsw_library.put_record(key, content)
        except Exception as e:
            print(pairing)
            print(f"Unsuccessful save: {e}")
            print("Critical Error - the above error completely prevents this embedding from saving to ResDB")
            print("this likely has ruined the entire embedding system. Please try to add your value again. If you face")
            print("the same error, delete all your saved data by deleting `vector-indexing/saved_data` and start fresh.")
            print("Terminating...")
            sys.exit()
    # (3/5) Create embedding information for the index passages file, which is ascii text data
    fileName = str(WORKING_DIR / "saved_data/temp/temp.ids.txt")
    key = embedding_keys["temp_ids_txt"]
    try:
        with open(fileName, 'r', encoding='ascii') as file:
            content = file.read()
            _ = hnsw_library.put_record(key, content)
    except Exception as e:
        print(f"Unsuccessful save: {e}")
        print("Critical Error - the above error completely prevents this embedding from saving to ResDB")
        print("this likely has ruined the entire embedding system. Please try to add your value again. If you face")
        print("the same error, delete all your saved data by deleting `vector-indexing/saved_data` and start fresh.")
        print("Terminating...")
        sys.exit()
    # (4/5) Create embedding information for the metadata file, which is a single json object
    fileName = str(WORKING_DIR / "saved_data/temp/temp.leann.meta.json")
    key = embedding_keys["temp_leann_meta_json"]
    try:
        with open(fileName, 'r') as file:
            content = json.load(file)
            _ =hnsw_library.put_record(key, content)
    except Exception as e:
        print(f"Unsuccessful save: {e}")
        print("Critical Error - the above error completely prevents this embedding from saving to ResDB")
        print("this likely has ruined the entire embedding system. Please try to add your value again. If you face")
        print("the same error, delete all your saved data by deleting `vector-indexing/saved_data` and start fresh.")
        print("Terminating...")
        sys.exit()
    # (5/5) Create embedding information for the passages file, which is a jsonLine file
    #   consisting of a single json object on each line
    fileName = str(WORKING_DIR / "saved_data/temp/temp.leann.passages.jsonl")
    key = embedding_keys["temp_leann_passages_json"]
    content = []
    try:
        with open(fileName, 'r') as file:
            # We load each json object line-by-line, saving each as an entry in an array
            for line in file:
                content.append(json.loads(line))
            _ = hnsw_library.put_record(key, content)
    except Exception as e:
        print(f"Unsuccessful save: {e}")
        print("Critical Error - the above error completely prevents this embedding from saving to ResDB")
        print("this likely has ruined the entire embedding system. Please try to add your value again. If you face")
        print("the same error, delete all your saved data by deleting `vector-indexing/saved_data` and start fresh.")
        print("Terminating...")
        sys.exit()

# - - - - - - - - - SECTION 5: Cleanup: Remove temporary files - - - - - - - - - >
    # Remove all temporary files created during HNSW Tree creation
    for file_temp_embedding in ["temp.ids.txt", "temp.index", "temp.leann.passages.idx",
        "temp.leann.meta.json", "temp.leann.passages.jsonl"]:
        fileName = str(WORKING_DIR / "saved_data/temp/" / file_temp_embedding)
        try:
            os.remove(fileName)
        except Exception as e:
            print(f"Error - A problem occurred while deleting temporary data: {e}")
            print("This is non-critical. It is reccomended you delete the folder `vector-indexing/saved_data/temp` to save space")

    # Remove the whole temp directory
    if os.path.exists(file_temporary_directory):
        file_temporary_directory.rmdir()




# = = = = = = = = = EXTRA SECTION: Future TODOs = = = = = = = = = >
# > The whole resdb_orm and config.yaml file had to be copied into the vector-indexing directory
#   See if there is a way to run this without them. One had to be installed as a package, it's
#   weird that it had to be duped in here to work
# > Look into the possibility of saving a value with custom keys in resdb-orm, instead of using
#   the random/autogenerated ones. This could ultimately lead to not needing to use a
#   saved_data/embedding_keys.json file at all