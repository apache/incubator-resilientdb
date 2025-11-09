from leann import LeannBuilder, LeannSearcher
from pathlib import Path
import os

# 1. Define the hash map (Python dictionary) to search
data_map = {
    # Original entries
    "doc1": "LEANN saves 97% storage compared to traditional vector databases.",
    "doc2": "Tung Tung Tung Sahur called—they need their banana-crocodile hybrid back",
    "doc3": "The weather in Davis is sunny today.",
    "doc4": "Understanding consensus protocols is key for blockchain.",

    # New entries (Course-related)
    "doc5": "ResilientDB is a high-throughput blockchain fabric designed for performance.",
    "doc6": "This project explores novel techniques for sharding in distributed ledgers.",
    "doc7": "DeFi applications are often built on top of smart contracts.",
    "doc8": "Practical Byzantine Fault Tolerance (PBFT) is a foundational agreement protocol.",
    "doc9": "Cross-chain communication enables interoperability between different blockchains.",
    "doc10": "The project requires using the ResilientDB Fabric unless approved otherwise.",

    # New entries (Unrelated noise)
    "doc11": "Mitochondria are the powerhouse of the cell.",
    "doc12": "How to bake a perfect sourdough bread with a starters.",
    "doc13": "The final report must be written in LaTeX using ACM templates.",
    "doc14": "UC Davis is known for its agricultural studies."
}

# 2. Create lists to map Leann's internal IDs (0, 1, 2...)
#    back to our original hash map keys.
#    map_keys[i] corresponds to map_values[i]
map_keys = list(data_map.keys())
map_values = list(data_map.values())

INDEX_PATH = str(Path("./hnsw-test").resolve() / "my_hashmap.leann")

# --- 3. Build the Leann Index ---
print("Building index with LeannBuilder...")
builder = LeannBuilder(backend_name="hnsw")

# Add the text values from the hash map to the builder.
# Leann will assign internal IDs starting from 0 (0, 1, 2, 3...)
for text in map_values:
    builder.add_text(text)

# Build and save the index file
builder.build_index(INDEX_PATH)
print(f"Index built and saved to {INDEX_PATH}")

# --- 4. Prepare the Leann Searcher ---
searcher = LeannSearcher(INDEX_PATH)

# 5. Create the semantic search function
def semantic_search_leann(query_text, k=3):
    """
    Uses LeannSearcher (vector indexing) to find the k-most
    semantically similar items from the hash map.
    """
    
    # searcher.search() returns a list of SearchResult objects
    results_from_leann = searcher.search(query_text, top_k=k)
    
    final_results = []
    if not results_from_leann:
        return final_results

    # Loop through the SearchResult objects
    for result in results_from_leann:
        
        # Get the internal ID (as an int) from the result object
        item_index = int(result.id) 
        
        # Use the ID to look up our original key and value
        key = map_keys[item_index]
        value = map_values[item_index]
        
        # Get the similarity score
        score = result.score 
        
        final_results.append({
            "key": key,
            "value": value,
            "similarity_score": score
        })
        
    return final_results

# --- 6. Run the search ---
print("\n--- Search Results (using leann) ---")

# First query
query1 = "Robust Database"
# Call the renamed function
results1 = semantic_search_leann(query1) 

if results1:
    print(f"Query: '{query1}'")
    # Print all results (since k=3 by default)
    for i, result in enumerate(results1):
        print(f"  Rank {i+1} ({result['key']}): {result['value']}")
        print(f"          (Score: {result['similarity_score']:.4f})")

print("---")