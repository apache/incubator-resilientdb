import requests
import json
import yaml
from pathlib import Path
import config

def diagnose():
    print("=== ResilientDB Diagnosis Tool ===")
    
    # 1. 設定ファイルからURLを取得
    try:
        with open(config.RESDB_CONFIG_PATH, 'r') as f:
            conf = yaml.safe_load(f)
        url = conf['database']['db_root_url']
        print(f"Target URL: {url}")
    except Exception as e:
        print(f"Error loading config: {e}")
        return

    # 2. 全データ取得のエンドポイントを叩く
    target_endpoint = f"{url}/v1/transactions"
    print(f"Requesting: {target_endpoint} ...")
    
    try:
        response = requests.get(target_endpoint)
        print(f"Status Code: {response.status_code}")
        
        # 生のレスポンス内容を表示
        content = response.text
        print(f"Raw Response Length: {len(content)}")
        print(f"Raw Response Preview (first 500 chars):\n{content[:500]}")
        
        if not content:
            print("\n[Error] Response body is EMPTY. The database returned no data.")
            print("Check if ResilientDB is running and if data was actually persisted.")
            return

        # JSONデコードを試行
        try:
            data = response.json()
            print(f"\nSuccess! Parsed JSON with {len(data)} records.")
            
            # doc1 があるか簡易チェック
            found_keys = []
            for tx in data:
                try:
                    if isinstance(tx.get('data'), str):
                        payload = json.loads(tx['data'])
                    else:
                        payload = tx.get('data')
                    
                    if isinstance(payload, dict) and 'original_key' in payload:
                        found_keys.append(payload['original_key'])
                except:
                    pass
            print(f"Found keys in DB: {found_keys}")
            
        except json.JSONDecodeError as e:
            print(f"\n[Error] JSON Decode Failed: {e}")
            print("The database response is not valid JSON.")

    except Exception as e:
        print(f"\n[Fatal Error] Request failed: {e}")

if __name__ == "__main__":
    diagnose()