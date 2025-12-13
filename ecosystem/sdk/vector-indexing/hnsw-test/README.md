# LEANN Semantic Search — Minimal Demo (README)
This README assumes a **single-file minimal demo** that turns a plain Python dict (`key → text`) into a semantic search using **LEANN** with an **HNSW** backend.  

## Overview
- **Goal**: Use LEANN to build an index and return the top-k semantically similar entries for a query.
- **Design**:
  - Single file (no OOP, no external mapping JSON)
  - Safe mapping from internal IDs back to your original `(key, text)`
  - Minimal, robust checks around IDs and `k`

## How It Works
- Prepare data: a Python dict ```data_map = { "doc1": "text...", ... }```.
- Freeze ordering: ```docs = list(data_map.items())``` so ```docs[i]``` matches LEANN’s internal ID ```i```.
- Build: feed texts into ```LeannBuilder(backend_name="hnsw")``` and save the index.
- Search: run ```LeannSearcher.search(query, top_k=k)``` to get ```(id, score)``` pairs.
- Map back: use ```id``` to recover the original ```(key, text)``` from ```docs[id]```.

## Prerequisites
- Python 3.12
- Package & venv management with **uv**

## Setup (with uv)
1. Install uv if needed:
```bash
# macOS / Linux
curl -LsSf https://astral.sh/uv/install.sh | sh

# Windows (PowerShell)
irm https://astral.sh/uv/install.ps1 | iex
```

2. Create a virtual environment and add dependencies:
```bash
uv venv
source .venv/bin/activate
uv pip install leann pathlib
```

## Run
```bash
python leann_simple_kvs.py
```
or
```
uv run python leann_simple_kvs.py
```

