"""
Filename: hnsw_library.py
Author(s) / Contrubtor(s): Steven Shoemaker / Regan Yang, Ritesh Patro, Yoshiki Yamaguchi, Tiching Kao
Date: 2025-Fall
Description: (Indexers project) Simple library to strongly type Indexer-Project embedding saves
"""
# Typical Python imports
from typing import Dict, List, Any
from pathlib import Path
# ResDB & HNSW imports
from resdb_orm.orm import ResDBORM

current_dir = Path(__file__).resolve().parent
# Abs path to resdb_orm/config.yaml 
config_path = current_dir / "resdb_orm" / "config.yaml"

if not config_path.exists():
    print(f"Warning: Config file not found at {config_path}")

db = ResDBORM(str(config_path))

# RETURNS: Key of the newly created record
def create_record(value: str | Dict[str, Any]) -> str:
    return db.create(value)

# RETURNS: Retrieved value (any of the 5 filetypes used by leann)
def get_record(key: str) -> str | Dict[str, Any] | List[Dict[str, Any]]:
    return db.read(key)

# RETURNS: True if update is successful, False otherwise
def put_record(key: str, value: str | Dict[str, Any]) -> bool:
    update_data = db.update(key, value)
    return update_data['status'] == 'update successful'




# = = = = = = = = = EXTRA SECTION: Future TODOs = = = = = = = = = >
# > Find a PyDoc / formal method of defining return type in function header