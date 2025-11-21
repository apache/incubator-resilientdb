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
    # Limit parallelism for deterministic behavior
    os.environ["OMP_NUM_THREADS"] = "1"
    os.environ["TOKENIZERS_PARALLELISM"] = "false"

    print(f"Indexer Service Started. Model: {config.MODEL_NAME}")
    print(f"Output Index: {config.INDEX_PATH}")
    
    db = SafeResDBORM(config_path=str(config.RESDB_CONFIG_PATH))
    last_tx_count = 0

    while True:
        try:
            all_txs = db.read_all()
            current_count = len(all_txs)

            # Start processing if transaction count increased
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
                        op = data.get('operation', 'upsert')
                        text = data.get('text', '')
                        
                        if key:
                            events.append({"key": key, "op": op, "text": text, "ts": ts, "id": str(tx['id'])})
                    except:
                        continue
                
                # 2. Sort by timestamp
                events.sort(key=lambda x: x['ts'])

                # 3. Replay state
                active_docs = {}
                for ev in events:
                    if ev['op'] == 'delete':
                        if ev['key'] in active_docs:
                            del active_docs[ev['key']]
                    else:
                        active_docs[ev['key']] = {
                            "text": ev['text'],
                            "resdb_id": ev['id'],
                            "original_key": ev['key']
                        }

                # 4. Build index
                valid_docs = list(active_docs.values())
                if valid_docs:
                    print(f"Rebuilding index for {len(valid_docs)} documents...")
                    
                    # --- Start timing ---
                    start_time = time.time()
                    
                    builder = LeannBuilder(backend_name="hnsw", model=config.MODEL_NAME)
                    for d in valid_docs:
                        builder.add_text(d['text'])
                    
                    # Save index
                    builder.build_index(str(config.INDEX_PATH))
                    
                    # --- End timing ---
                    end_time = time.time()
                    elapsed_time = end_time - start_time
                    
                    # Save mapping
                    mapping_data = [{
                        "resdb_id": d['resdb_id'],
                        "original_key": d['original_key'],
                        "preview": d['text'][:60]
                    } for d in valid_docs]
                    
                    with open(config.MAPPING_PATH, 'w') as f:
                        json.dump(mapping_data, f, indent=2)
                        
                    print(f"Index updated successfully at {time.ctime()}")
                    print(f"Time taken: {elapsed_time:.4f} seconds") # Display time taken here
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