import argparse
import json
import time
import requests
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
                response = requests.get(url, timeout=5)
                
                if response.status_code == 200:
                    return response.json() if response.content else []
                elif response.status_code == 503:
                    time.sleep(2) 
                    continue
            except Exception:
                time.sleep(1)
        return []

def get_db():
    try:
        return SafeResDBORM(config_path=str(config.RESDB_CONFIG_PATH))
    except Exception as e:
        print(f"Connection failed: {e}")
        return None

def get_active_keys(db):
    """
    Fetches all transactions to determine which keys currently exist.
    Used for validation warnings.
    """
    all_txs = db.read_all()
    events = []
    
    for tx in all_txs:
        try:
            data = tx.get('data')
            if isinstance(data, str):
                try: data = json.loads(data)
                except: data = {"text": data}
            
            if not isinstance(data, dict): continue
            
            ts = float(data.get('timestamp', 0))
            key = data.get('original_key')
            op = data.get('operation', 'upsert')
            
            if key:
                events.append({"key": key, "op": op, "ts": ts})
        except:
            continue

    events.sort(key=lambda x: x['ts'])

    active_keys = set()
    for ev in events:
        if ev['op'] == 'delete':
            active_keys.discard(ev['key'])
        elif ev['op'] == 'update':
            pass 
        else:
            active_keys.add(ev['key'])
            
    return active_keys

def add_event(key, text, op_type):
    """
    Logs an event to ResilientDB.
    Performs a soft validation: Warns if key is missing but allows execution
    to handle eventual consistency (lag).
    """
    db = get_db()
    if not db: return

    # --- SOFT VALIDATION LOGIC ---
    if op_type in ["update", "delete"]:
        print(f"Checking key status for '{key}'...")
        active_keys = get_active_keys(db)
        
        if key not in active_keys:
            print(f"\n[WARNING] Key '{key}' was not found in the current database state.")
            print("  -> If you JUST created this key, this is normal (propagation lag). Proceeding...")
            print("  -> If this is a typo, the update will be IGNORED by the indexer.\n")
            # We do NOT return here; we proceed to send the transaction.
        else:
            print(f"Key '{key}' found. Proceeding with {op_type}.")
    # -----------------------------

    payload = {
        "original_key": key,
        "text": text,
        "timestamp": time.time(),
        "operation": op_type,
        "type": "vector_source"
    }
    
    time.sleep(0.5)
    
    try:
        tx_id = db.create(payload)
        if isinstance(tx_id, str):
            # Message changed to "Request Sent" to be accurate
            print(f"[{op_type.upper()} REQUEST SENT] Key: '{key}' (Tx: {tx_id[:8]}...)")
        else:
            print(f"[{op_type.upper()}] Failed: {tx_id}")
    except Exception as e:
        print(f"Error sending transaction: {e}")

def main():
    parser = argparse.ArgumentParser(description="ResilientDB Vector Data Manager")
    subparsers = parser.add_subparsers(dest="command", required=True)

    p_add = subparsers.add_parser("add")
    p_add.add_argument("key")
    p_add.add_argument("text")

    p_upd = subparsers.add_parser("update")
    p_upd.add_argument("key")
    p_upd.add_argument("text")

    p_del = subparsers.add_parser("delete")
    p_del.add_argument("key")

    args = parser.parse_args()

    if args.command == "add":
        add_event(args.key, args.text, "add")
    elif args.command == "update":
        add_event(args.key, args.text, "update")
    elif args.command == "delete":
        add_event(args.key, "", "delete")

if __name__ == "__main__":
    main()