import json
import os
import time
import sys
from pathlib import Path
from leann import LeannSearcher
import config

class AutoSearcher:
    def __init__(self):
        self.searcher = None
        self.id_mapping = []
        self.last_mtime = 0
        self.index_file = self._resolve_index_file()
        self.mapping_file = config.MAPPING_PATH
        self.reload_index()

    def _resolve_index_file(self):
        """実際のインデックスファイルパスを解決する"""
        # leannは .index を付与する場合があるためチェック
        candidates = [
            config.INDEX_PATH,
            config.INDEX_PATH.with_suffix(".index"),
            Path(str(config.INDEX_PATH) + ".index")
        ]
        for p in candidates:
            if p.exists():
                return p
        return config.INDEX_PATH # デフォルト

    def _get_mtime(self):
        """マッピングファイルの更新時刻を取得（これが更新トリガーとなる）"""
        if self.mapping_file.exists():
            return self.mapping_file.stat().st_mtime
        return 0

    def reload_index(self):
        """インデックスの再ロード"""
        if not self.index_file.exists() or not self.mapping_file.exists():
            return False

        try:
            # マッピングの読み込み
            with open(self.mapping_file, 'r') as f:
                self.id_mapping = json.load(f)
            
            # インデックスのロード (存在しない場合はスキップ)
            if self.id_mapping:
                self.searcher = LeannSearcher(str(config.INDEX_PATH), model=config.MODEL_NAME)
            else:
                self.searcher = None

            self.last_mtime = self._get_mtime()
            print(f"\n[System] Index reloaded. Documents: {len(self.id_mapping)}")
            return True
        except Exception as e:
            print(f"\n[Error] Failed to reload index: {e}")
            return False

    def search(self, query):
        # 検索前に更新チェック
        current_mtime = self._get_mtime()
        if current_mtime > self.last_mtime:
            print("\n[System] Detected update. Refreshing index...")
            time.sleep(0.5) # 書き込み完了待ち
            self.reload_index()

        if not self.searcher or not self.id_mapping:
            return []

        try:
            results = self.searcher.search(query, top_k=3)
            return results
        except Exception as e:
            print(f"Search execution error: {e}")
            return []

    def get_info(self, result_id):
        try:
            idx = int(result_id)
            if 0 <= idx < len(self.id_mapping):
                return self.id_mapping[idx]
        except:
            pass
        return None

def main():
    print("=== ResilientDB Auto-Reloading Search CLI ===")
    engine = AutoSearcher()

    if not engine.index_file.exists():
        print("Waiting for initial index creation...")

    while True:
        try:
            query = input("\nSearch Query ('exit' to quit): ").strip()
            if not query: continue
            if query.lower() in ['exit', 'quit']: break

            results = engine.search(query)

            if not results:
                print("No results found.")
                continue

            print(f"Results for: '{query}'")
            for rank, res in enumerate(results, 1):
                info = engine.get_info(res.id)
                if info:
                    print(f"  #{rank} [Score: {res.score:.4f}]")
                    print(f"    Key : {info['original_key']}")
                    print(f"    Text: {info['preview']}...")
                else:
                    print(f"  #{rank} [Unknown ID]")

        except KeyboardInterrupt:
            print("\nBye!")
            break

if __name__ == "__main__":
    main()