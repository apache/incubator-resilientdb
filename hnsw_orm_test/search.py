import json
import sys
import os
from leann import LeannSearcher
import config

def main():
    # Check index existence (Check actual existence of .index file)
    real_index_file = config.INDEX_PATH.with_suffix(".index")
    
    if not real_index_file.exists() or not config.MAPPING_PATH.exists():
        print(f"Index not found at {real_index_file}")
        print("Please run 'python populate.py' then 'python indexer.py' first.")
        return

    print(f"Loading index from {config.INDEX_PATH}...")
    print(f"Using model: {config.MODEL_NAME}")

    try:
        # Initialize searcher specifying the lightweight model
        searcher = LeannSearcher(
            str(config.INDEX_PATH), 
            model=config.MODEL_NAME
        )
        
        with open(config.MAPPING_PATH, 'r') as f:
            id_mapping = json.load(f)
            
        print(f"Loaded {len(id_mapping)} documents mapping.")
    except Exception as e:
        print(f"Failed to load index: {e}")
        return

    print("\n=== LEANN (bert-tiny) x ResilientDB Search CLI ===")
    print("Type 'exit' to quit.")

    while True:
        try:
            query = input("\nSearch Query: ").strip()
            if not query: continue
            if query.lower() in ['exit', 'quit']: break

            # Execute search
            k = min(3, len(id_mapping))
            results = searcher.search(query, top_k=k)

            if not results:
                print("No results found.")
                continue

            print(f"Results for: '{query}'")
            for rank, res in enumerate(results, 1):
                try:
                    leann_id = int(res.id)
                    if 0 <= leann_id < len(id_mapping):
                        info = id_mapping[leann_id]
                        score = float(res.score)
                        
                        print(f"  #{rank} [Score: {score:.4f}]")
                        print(f"    Key: {info['original_key']} (ResDB ID: {info['resdb_id']})")
                        print(f"    Text: {info['preview']}...")
                except ValueError:
                    continue

        except KeyboardInterrupt:
            print("\nBye!")
            break
        except Exception as e:
            print(f"Search error: {e}")

if __name__ == "__main__":
    main()