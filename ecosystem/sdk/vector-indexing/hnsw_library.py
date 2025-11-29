from resdb_orm.orm import ResDBORM
from leann import LeannBuilder, LeannSearcher
from pathlib import Path
from typing import Dict, List, Any

db = ResDBORM()

def create_record(value: str | Dict[str, Any]) -> str:
    return db.create(value)

def get_record(key: str) -> str | Dict[str, Any] | List[Dict[str, Any]]:
    return db.read(key)

#TODO: Replace none with successful or not successful (youll have to check the return type of update)
def put_record(key: str, value: str | Dict[str, Any]) -> None:
    _ = db.update(key, value)

# - - - - - - - - - FINAL SECTION: LONG-TERM TODO s - - - - - - - - - >
# TODO: strongly type as much as you can