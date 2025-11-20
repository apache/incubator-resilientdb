import os
from pathlib import Path

# --- Environment Settings ---
# Path to the ResDB-ORM configuration file (Auto-detection)
RESDB_CONFIG_PATH = Path.home() / "ResDB-ORM" / "config.yaml"
if not RESDB_CONFIG_PATH.exists():
    potential_local_path = Path("config.yaml").resolve()
    if potential_local_path.exists():
        RESDB_CONFIG_PATH = potential_local_path
    else:
        env_path = os.getenv("RESDB_CONFIG_FILE")
        if env_path:
            RESDB_CONFIG_PATH = Path(env_path)

# --- Model Settings (Lightweight!) ---
# prajjwal1/bert-tiny: Approx. 17MB, 128 dimensions
MODEL_NAME = "prajjwal1/bert-tiny"

# --- Directory Settings ---
# Directory where data will be saved
BASE_DIR = Path("./leann_resdb_tiny").resolve()
BASE_DIR.mkdir(parents=True, exist_ok=True)

# Path to the index (Logical path)
# In reality, files like resdb.index will be generated
INDEX_PATH = BASE_DIR / "resdb.leann"
MAPPING_PATH = BASE_DIR / "id_mapping.json"

# Polling interval (seconds)
POLL_INTERVAL = 5

print(f"Config: Using model '{MODEL_NAME}' at {BASE_DIR}")