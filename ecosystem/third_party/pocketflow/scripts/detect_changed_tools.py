#!/usr/bin/env python3
"""
Detect which tools have changed in the current commit/push
by checking git diff against the tool-doc-map.json.

Usage:
    python ecosystem/pocketflow/scripts/detect_changed_tools.py [--base-ref <ref>]

Returns a list of tool names (one per line) that have changed.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

# Repo root is three levels up from this script
REPO_ROOT = Path(__file__).resolve().parents[3]
POCKETFLOW_DIR = REPO_ROOT / "ecosystem" / "pocketflow"
TOOL_MAP_PATH = POCKETFLOW_DIR / "tool-doc-map.json"

# Exclude these paths from triggering updates
EXCLUDE_PATTERNS = [
    "ecosystem/AI-Tooling/beacon/**",  # Don't trigger on doc changes
    "ecosystem/pocketflow/**",  # Don't trigger on pocketflow changes
]


def load_tool_map() -> list[dict]:
    """Load the tool-doc-map.json file."""
    try:
        with TOOL_MAP_PATH.open("r", encoding="utf-8") as fh:
            return json.load(fh)
    except FileNotFoundError as exc:
        raise SystemExit(f"tool map not found at {TOOL_MAP_PATH}") from exc


def get_changed_files(base_ref: str = "HEAD^") -> set[str]:
    """Get list of changed files between base_ref and HEAD."""
    try:
        # Get diff between base_ref and HEAD
        result = subprocess.run(
            ["git", "diff", "--name-only", base_ref, "HEAD"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=True,
        )
        changed = set(result.stdout.strip().split("\n"))
        # Filter out empty strings
        changed.discard("")
        return changed
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f"git diff failed: {exc}") from exc


def should_exclude_path(path: str) -> bool:
    """Check if a path should be excluded from triggering updates."""
    for pattern in EXCLUDE_PATTERNS:
        # Simple pattern matching (supports ** for recursive)
        if pattern.endswith("/**"):
            prefix = pattern[:-3]  # Remove /**
            if path.startswith(prefix + "/") or path == prefix:
                return True
        elif path == pattern or path.startswith(pattern + "/"):
            return True
    return False


def detect_changed_tools(base_ref: str = "HEAD^") -> list[str]:
    """
    Detect which tools have changed by checking if their code_root paths
    have any changed files.
    """
    tool_map = load_tool_map()
    changed_files = get_changed_files(base_ref)
    
    # Filter out excluded paths
    relevant_changes = {
        f for f in changed_files if not should_exclude_path(f)
    }
    
    if not relevant_changes:
        return []
    
    # Find which tools have changes in their code_root
    changed_tools = []
    for entry in tool_map:
        tool_name = entry.get("name")
        code_root = entry.get("code_root")
        
        if not code_root or not tool_name:
            continue
        
        # Check if any changed file is under this tool's code_root
        for changed_file in relevant_changes:
            if changed_file.startswith(code_root + "/") or changed_file == code_root:
                changed_tools.append(tool_name)
                break  # Only add once per tool
    
    return changed_tools


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Detect which tools have changed in git commits."
    )
    parser.add_argument(
        "--base-ref",
        default="HEAD^",
        help="Base reference for comparison (default: HEAD^, i.e., previous commit)",
    )
    args = parser.parse_args()
    
    changed_tools = detect_changed_tools(args.base_ref)
    
    # Output one tool name per line
    for tool in changed_tools:
        print(tool)


if __name__ == "__main__":
    main()

