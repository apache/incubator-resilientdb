import time
import json
import os
import gc
import requests
from resdb_orm.orm import ResDBORM
from leann import LeannBuilder
import config

class SafeResDBORM(ResDBORM):
    def read_all(self):
        try:
            url = f'{self.db_root_url}/v1/transactions'
            response = requests.get(url, timeout=10)
            if response.status_code == 200 and response.content:
                return response.json()
            return []
        except:
            return []

def main():
    os.environ["OMP_NUM_THREADS"] = "1"
    os.environ["TOKENIZERS_PARALLELISM"] = "false"

    print(f"Indexer Service Started. Model: {config.MODEL_NAME}")
    
    db = SafeResDBORM(config_path=str(config.RESDB_CONFIG_PATH))
    last_tx_count = 0

    while True:
        try:
            all_txs = db.read_all()
            current_count = len(all_txs)

            if current_count > last_tx_count:
                print(f"\n[Change Detected] {last_tx_count} -> {current_count} transactions.")
                
                # 1. Extract events
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
                        # If the operation field is missing, it is treated as 'upsert' (forced overwrite).
                        op = data.get('operation', 'upsert')
                        text = data.get('text', '')
                        
                        if key:
                            events.append({"key": key, "op": op, "text": text, "ts": ts, "id": str(tx['id'])})
                    except:
                        continue
                
                # 2. Sort by timestamp
                events.sort(key=lambda x: x['ts'])

                # 3. Replay state (Filtering Logic)
                active_docs = {}
                for ev in events:
                    key = ev['key']
                    op = ev['op']
                    
                    if op == 'delete':
                        if key in active_docs:
                            del active_docs[key]
                    elif op == 'update':
                        # Reload if the key exists
                        if key in active_docs:
                            active_docs[key] = {
                                "text": ev['text'],
                                "resdb_id": ev['id'],
                                "original_key": key
                            }
                        else:
                            # Ignore updates to non-existent keys and log them.
                            print(f"Warning: Ignored 'update' for non-existent key: '{key}'")
                    else:
                        # 'add' saves unconditionally
                        active_docs[key] = {
                            "text": ev['text'],
                            "resdb_id": ev['id'],
                            "original_key": key
                        }

                # 4. Build index
                valid_docs = list(active_docs.values())
                if valid_docs:
                    print(f"Rebuilding index for {len(valid_docs)} documents...")
                    
                    start_time = time.time()
                    builder = LeannBuilder(backend_name="hnsw", model=config.MODEL_NAME)
                    for d in valid_docs:
                        builder.add_text(d['text'])
                    
                    builder.build_index(str(config.INDEX_PATH))
                    
                    elapsed_time = time.time() - start_time
                    
                    mapping_data = [{
                        "resdb_id": d['resdb_id'],
                        "original_key": d['original_key'],
                        "preview": d['text'][:60]
                    } for d in valid_docs]
                    
                    with open(config.MAPPING_PATH, 'w') as f:
                        json.dump(mapping_data, f, indent=2)
                        
                    print(f"Index updated. Time: {elapsed_time:.4f}s")
                else:
                    print("Index cleared (no active documents).")
                    with open(config.MAPPING_PATH, 'w') as f:
                        json.dump([], f)

                last_tx_count = current_count
                if 'builder' in locals(): del builder
                gc.collect()

        except Exception as e:
            print(f"Polling error: {e}")

        time.sleep(config.POLL_INTERVAL)

if __name__ == "__main__":
    main()