# vector_client.py
import argparse
import requests
import sys

# Proxy Server URL
# Use 'localhost' for local testing
# Use the external IP (e.g., "http://34.xx.xx.xx:5000") for cloud deployment
PROXY_URL = "http://localhost:5000"

def cmd_add(args):
    """Send add request to proxy"""
    print(f"Adding value: '{args.value}'...")
    try:
        resp = requests.post(f"{PROXY_URL}/add", json={"text": args.value})
        data = resp.json()
        
        if resp.status_code == 200:
            print(f"[SUCCESS] {data.get('message')}")
        else:
            print(f"[ERROR] {data.get('message') or data.get('error')}")
    except Exception as e:
        print(f"Connection failed: {e}")

def cmd_delete(args):
    """Send delete request to proxy"""
    print(f"Deleting value: '{args.value}'...")
    try:
        resp = requests.post(f"{PROXY_URL}/delete", json={"text": args.value})
        data = resp.json()

        if resp.status_code == 200:
            print(f"[SUCCESS] {data.get('message')}")
        else:
            print(f"[ERROR] {data.get('error')}")
    except Exception as e:
        print(f"Connection failed: {e}")

def cmd_search(args):
    """Send search request to proxy"""
    print(f"Searching for: '{args.value}' (Top {args.k_matches})...")
    try:
        resp = requests.post(f"{PROXY_URL}/search", json={"value": args.value, "k": args.k_matches})
        data = resp.json()

        if resp.status_code == 200:
            results = data.get("results", [])
            print(f"\n--- Found {len(results)} results ---")
            if not results:
                print("No matches found.")
            for i, item in enumerate(results, 1):
                print(f"{i}. {item['text']} (Score: {item['score']:.4f})")
            print("----------------------------")
        else:
            print(f"[ERROR] {data.get('error')}")
    except Exception as e:
        print(f"Connection failed: {e}")

def main():
    parser = argparse.ArgumentParser(description="Vector Search Client")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # Add command
    p_add = subparsers.add_parser("add")
    p_add.add_argument("--value", required=True)
    p_add.set_defaults(func=cmd_add)

    # Delete command
    p_del = subparsers.add_parser("delete")
    p_del.add_argument("--value", required=True)
    p_del.set_defaults(func=cmd_delete)

    # Search command
    p_search = subparsers.add_parser("search")
    p_search.add_argument("--value", required=True)
    p_search.add_argument("--k_matches", type=int, default=3)
    p_search.set_defaults(func=cmd_search)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()