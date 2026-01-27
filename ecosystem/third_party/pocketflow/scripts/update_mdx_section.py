#!/usr/bin/env python3
"""
Replace the auto-generated documentation block inside an MDX file
with fresh content produced by Pocketflow.

Usage:
    python ecosystem/pocketflow/scripts/update_mdx_section.py \
        --tool rescontract \
        --body-file /tmp/rescontract_body.md
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

# Repo root is three levels up from this script:
# ecosystem/pocketflow/scripts/update_mdx_section.py
REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_TEMPLATE = """# {title}

{{/* BEGIN AUTO_DOC: {name} */}}

<!-- Pocketflow will overwrite this block -->

{{/* END AUTO_DOC: {name} */}}
"""


def load_tool_map() -> list[dict]:
    tool_map_path = REPO_ROOT / "ecosystem" / "pocketflow" / "tool-doc-map.json"
    try:
        with tool_map_path.open("r", encoding="utf-8") as fh:
            return json.load(fh)
    except FileNotFoundError as exc:
        raise SystemExit(f"tool map not found at {tool_map_path}") from exc


def find_tool_entry(tool_name: str, tool_map: list[dict]) -> dict:
    for entry in tool_map:
        if entry.get("name") == tool_name:
            return entry
    raise SystemExit(f"tool '{tool_name}' missing from tool-doc-map.json")


def ensure_doc_file(path: Path, tool_name: str) -> None:
    if path.exists():
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    template = DEFAULT_TEMPLATE.format(name=tool_name, title=tool_name.title())
    with path.open("w", encoding="utf-8") as fh:
        fh.write(template.strip() + "\n")
    print(f"[update-mdx] Created placeholder template at {path}")


def replace_block(original: str, tool_name: str, body: str) -> str:
    start_marker = f"{{/* BEGIN AUTO_DOC: {tool_name} */}}"
    end_marker = f"{{/* END AUTO_DOC: {tool_name} */}}"

    start_idx = original.find(start_marker)
    end_idx = original.find(end_marker)

    if start_idx == -1 or end_idx == -1:
        raise SystemExit(
            f"Markers not found in MDX file. Expected '{start_marker}' and '{end_marker}'."
        )
    if end_idx <= start_idx:
        raise SystemExit("END marker appears before BEGIN marker.")

    before = original[: start_idx + len(start_marker)]
    after = original[end_idx:]

    body_normalized = "\n\n" + body.strip() + "\n\n"
    return before + body_normalized + after


def run_prettier(path: Path) -> None:
    cmd = ["npx", "prettier", "--write", str(path)]
    try:
        completed = subprocess.run(
            cmd, cwd=REPO_ROOT, capture_output=True, text=True, check=True
        )
        if completed.stdout:
            print(completed.stdout.strip())
        if completed.stderr:
            print(completed.stderr.strip())
    except FileNotFoundError:
        print("[update-mdx] Prettier not found (npx missing). Skipping formatting.")
    except subprocess.CalledProcessError as exc:
        print(f"[update-mdx] Prettier failed: {exc.stderr.strip()}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Update MDX auto-doc section.")
    parser.add_argument("--tool", required=True, help="Tool name in tool-doc-map.json")
    parser.add_argument(
        "--body-file", required=True, help="Path to file containing markdown body"
    )
    parser.add_argument(
        "--skip-format",
        action="store_true",
        help="Skip running Prettier after updating the MDX file.",
    )
    args = parser.parse_args()

    body_path = Path(args.body_file)
    if not body_path.exists():
        raise SystemExit(f"Body file not found: {body_path}")

    tool_map = load_tool_map()
    entry = find_tool_entry(args.tool, tool_map)

    doc_rel_path = entry.get("doc_path")
    if not doc_rel_path:
        raise SystemExit(f"'doc_path' missing for tool {args.tool}")
    doc_path = REPO_ROOT / doc_rel_path

    ensure_doc_file(doc_path, args.tool)
    body = body_path.read_text(encoding="utf-8")
    original = doc_path.read_text(encoding="utf-8")
    updated = replace_block(original, args.tool, body)
    doc_path.write_text(updated, encoding="utf-8")
    print(f"[update-mdx] Updated {doc_path}")

    if not args.skip_format:
        run_prettier(doc_path)


if __name__ == "__main__":
    try:
        main()
    except SystemExit as exc:
        print(exc, file=sys.stderr)
        raise

