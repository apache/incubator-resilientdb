import argparse
import json
import time
import requests
import re
from resdb_orm.orm import ResDBORM
import config

class SafeResDBORM(ResDBORM):
    """
    A wrapper class for ResDBORM that includes error handling 
    and retry logic for network requests.
    """
    def read_all(self):
        max_retries = 3
        for attempt in range(max_retries):
            try:
                url = f'{self.db_root_url}/v1/transactions'
                # Set a timeout to prevent hanging on connection issues
                response = requests.get(url, timeout=5)
                
                if response.status_code == 200:
                    # Return JSON if content exists, otherwise return an empty list
                    return response.json() if response.content else []
                elif response.status_code == 503:
                    # Handle server cooldown (Service Unavailable)
                    time.sleep(2) 
                    continue
            except Exception:
                # Wait briefly before retrying on generic errors
                time.sleep(1)
        return []

def get_db():
    """Initializes and returns the SafeResDBORM instance."""
    try:
        return SafeResDBORM(config_path=str(config.RESDB_CONFIG_PATH))
    except Exception as e:
        print(f"Connection failed: {e}")
        return None

def add_event(key, text, op_type):
    """
    Logs an event (add, update, or delete) to ResilientDB using Event Sourcing.
    Instead of overwriting data, we append a new transaction with a timestamp.
    """
    db = get_db()
    if not db: return

    # Construct the payload. 
    # 'timestamp' is crucial for the indexer to determine the latest state.
    payload = {
        "original_key": key,
        "text": text,
        "timestamp": time.time(),
        "operation": op_type,
        "type": "vector_source"
    }
    
    # Brief pause to mitigate server load during rapid operations
    time.sleep(0.5)
    
    try:
        # Create a new transaction (append-only)
        tx_id = db.create(payload)
        if isinstance(tx_id, str):
            print(f"[{op_type.upper()}] Key: '{key}' (Tx: {tx_id[:8]}...)")
        else:
            print(f"[{op_type.upper()}] Failed: {tx_id}")
    except Exception as e:
        print(f"Error sending transaction: {e}")

def main():
    # Set up CLI argument parsing
    parser = argparse.ArgumentParser(description="ResilientDB Vector Data Manager")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # Command: Add
    p_add = subparsers.add_parser("add")
    p_add.add_argument("key")
    p_add.add_argument("text")

    # Command: Update
    p_upd = subparsers.add_parser("update")
    p_upd.add_argument("key")
    p_upd.add_argument("text")

    # Command: Delete
    p_del = subparsers.add_parser("delete")
    p_del.add_argument("key")

    args = parser.parse_args()

    # Execute the corresponding event function based on the command
    if args.command == "add":
        add_event(args.key, args.text, "add")
    elif args.command == "update":
        add_event(args.key, args.text, "update")
    elif args.command == "delete":
        # For deletion, text content is irrelevant, so we send an empty string
        add_event(args.key, "", "delete")

if __name__ == "__main__":
    main()