import time
from resdb_orm.orm import ResDBORM
import config  # Import common settings

# Data to populate (Hash map)
data_map = {
    "doc1": "LEANN saves 97% storage compared to traditional vector databases.",
    "doc2": "Tung Tung Tung Sahur called—they need their banana-crocodile hybrid back",
    "doc3": "The weather in Davis is sunny today.",
    "doc4": "Understanding consensus protocols is key for blockchain.",
    "doc5": "ResilientDB is a high-throughput blockchain fabric designed for performance.",
    "doc6": "This project explores novel techniques for sharding in distributed ledgers.",
    "doc7": "DeFi applications are often built on top of smart contracts.",
    "doc8": "Practical Byzantine Fault Tolerance (PBFT) is a foundational agreement protocol.",
    "doc9": "Cross-chain communication enables interoperability between different blockchains.",
    "doc10": "The project requires using the ResilientDB Fabric unless approved otherwise.",
    "doc11": "Mitochondria are the powerhouse of the cell.",
    "doc12": "How to bake a perfect sourdough bread with a starters.",
    "doc13": "The final report must be written in LaTeX using ACM templates.",
    "doc14": "UC Davis is known for its agricultural studies."
}

def main():
    print(f"Connecting to ResilientDB via {config.RESDB_CONFIG_PATH}...")
    try:
        db = ResDBORM(config_path=str(config.RESDB_CONFIG_PATH))
    except Exception as e:
        print(f"Connection failed: {e}")
        return

    print(f"Starting ingestion of {len(data_map)} documents...")

    for key, text in data_map.items():
        # Insert in a format easy for the indexer to recognize
        payload = {
            "text": text,
            "original_key": key,
            "type": "vector_source"
        }

        try:
            tx_id = db.create(payload)
            print(f"Stored '{key}': {tx_id}")
            time.sleep(0.2) # Short sleep to reduce load
        except Exception as e:
            print(f"Failed to store {key}: {e}")

    print("\n Data population complete!")

if __name__ == "__main__":
    main()