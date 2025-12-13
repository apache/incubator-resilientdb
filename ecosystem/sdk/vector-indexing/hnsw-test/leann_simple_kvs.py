from leann import LeannBuilder, LeannSearcher
from pathlib import Path

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

# 2. Make a single source of groundtruth for ID, key, and value
docs = list(data_map.items())

INDEX_PATH = Path("./hnsw-test").resolve() / "my_hashmap.leann"
INDEX_PATH.parent.mkdir(parents=True, exist_ok=True)

# --- 3. Build the Leann Index ---
print("Building index with LeannBuilder...")
builder = LeannBuilder(backend_name="hnsw")

# Add the text from the docs to the builder.
for _, text in docs:
    builder.add_text(text)

# Build and save the index file
builder.build_index(INDEX_PATH)
print(f"Index built and saved to {INDEX_PATH}")

# --- 4. Prepare the Leann Searcher ---
searcher = LeannSearcher(str(INDEX_PATH))

# 5. Create the semantic search function
def semantic_search_leann(query_text: str, k: int = 3):
    """
    Uses LeannSearcher (vector indexing) to find the k-most
    semantically similar items from the hash map.
    """
    if not query_text:
        return []
    
    k = max(0, min(k, len(docs)))
    if k == 0:
        return []

    results_from_leann = searcher.search(query_text, top_k=k)
    final_results = []
    if not results_from_leann:
        return final_results

    # Loop through the SearchResult objects
    for result in results_from_leann:
        
        # Get the internal ID (as an int) from the result object
        try: 
            item_index = int(result.id)
        except:
            continue 
        
        if not (0 <= item_index < len(docs)):
            continue
        
        key, value = docs[item_index]       
        
        # Get the similarity score
        score = float(result.score) 
        
        final_results.append({
            "key": key,
            "value": value,
            "similarity_score": score
        })
        
    return final_results

# --- 6. Show the result ---
def print_results(query: str, results):
    if not results:
        print(f"No results for query: {query!r}")
        return
    
    print(f"Query: '{query}'")
    
    for i, r in enumerate(results, 1):
        print(f"  Rank {i} ({r['key']}): {r['value']}")
        print(f"          (Score: {r['similarity_score']:.4f})")

# --- 7. Run the search ---
print("\n--- Search Results (using leann) ---")
query1 = "Robust Database"
results1 = semantic_search_leann(query1, k=3)
print_results(query1, results1)
print("---")