import time
import json
import os
import gc
from resdb_orm.orm import ResDBORM
from leann import LeannBuilder
import config

def main():
    # Limit parallelism for memory saving and determinism
    os.environ["OMP_NUM_THREADS"] = "1"
    os.environ["TOKENIZERS_PARALLELISM"] = "false"

    print(f"Indexer started with lightweight model: {config.MODEL_NAME}")
    print(f"Watching ResilientDB ({config.RESDB_CONFIG_PATH})...")
    
    try:
        db = ResDBORM(config_path=str(config.RESDB_CONFIG_PATH))
    except Exception as e:
        print(f"DB Connection failed: {e}")
        return

    last_count = 0

    while True:
        try:
            # 1. Fetch all data
            all_txs = db.read_all()
            
            if not all_txs:
                print("  No data yet. Waiting...")
                time.sleep(config.POLL_INTERVAL)
                continue

            current_count = len(all_txs)

            # 2. Change detection (Simple implementation)
            if current_count > last_count:
                print(f"\nDetected change: {last_count} -> {current_count} records.")
                
                # Sort by ID for determinism
                sorted_txs = sorted(all_txs, key=lambda x: str(x.get('id', '')))

                docs_text = []
                docs_mapping = []

                for tx in sorted_txs:
                    try:
                        data = tx.get('data', {})
                        if isinstance(data, str):
                            try: data = json.loads(data)
                            except: data = {"text": data}
                        
                        text = data.get('text', '')
                        if text:
                            docs_text.append(text)
                            docs_mapping.append({
                                "resdb_id": str(tx['id']),
                                "original_key": data.get('original_key', 'unknown'),
                                "preview": text[:60]
                            })
                    except Exception:
                        continue

                if docs_text:
                    print(f"Rebuilding index for {len(docs_text)} documents...")
                    
                    # 3. Build index (Specify lightweight model)
                    builder = LeannBuilder(
                        backend_name="hnsw", 
                        model=config.MODEL_NAME
                    )
                    
                    for t in docs_text:
                        builder.add_text(t)
                    
                    builder.build_index(str(config.INDEX_PATH))
                    
                    # Save mapping
                    with open(config.MAPPING_PATH, 'w') as f:
                        json.dump(docs_mapping, f, indent=2)
                        
                    print(f"✅ Index updated at {config.INDEX_PATH}")
                    last_count = current_count
                    
                    # Free memory (Important)
                    del builder
                    gc.collect()
                else:
                    print("No valid text documents found.")

            else:
                # No changes
                pass

        except Exception as e:
            print(f"Error in polling loop: {e}")

        time.sleep(config.POLL_INTERVAL)

if __name__ == "__main__":
    main()