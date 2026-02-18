#!/usr/bin/env python3
"""
Run Pocketflow for a single tool and update its Beacon MDX page
using only the generated index.md content.

Usage (from repo root):

    python ecosystem/pocketflow/scripts/run_pocketflow_for_tool.py --tool rescontract

This will:
  - Look up `rescontract` in tool-doc-map.json
  - Run Pocketflow against its `code_root`
  - Read the generated `index.md` for that project
  - Inject the content into the MDX auto-doc block for that tool
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

# Repo root is three levels up from this script:
# ecosystem/pocketflow/scripts/run_pocketflow_for_tool.py
REPO_ROOT = Path(__file__).resolve().parents[3]
POCKETFLOW_DIR = REPO_ROOT / "ecosystem" / "pocketflow"
OUTPUT_BASE = POCKETFLOW_DIR / ".pf-output"


def load_tool_map() -> list[dict]:
    tool_map_path = POCKETFLOW_DIR / "tool-doc-map.json"
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


def run_pocketflow_for_dir(
    code_root: Path,
    output_dir: Path,
    language: str = "english",
    use_cache: bool = True,
    max_abstractions: int = 10,
    single_file: bool = False,
) -> Path:
    """
    Invoke ecosystem/pocketflow/main.py pointing at code_root and
    returning the expected path to index.md for the generated project.
    """
    main_py = POCKETFLOW_DIR / "main.py"
    if not main_py.exists():
        raise SystemExit(f"Pocketflow main.py not found at {main_py}")

    project_name = os.path.basename(str(code_root))
    output_dir.mkdir(parents=True, exist_ok=True)

    cmd = [
        sys.executable,
        str(main_py),
        "--dir",
        str(code_root),
        "-o",
        str(output_dir),
        "--language",
        language,
        "--max-abstractions",
        str(max_abstractions),
    ]
    if single_file:
        cmd.append("--single-file")  # Generate single consolidated file instead of index + chapters
    if not use_cache:
        cmd.append("--no-cache")

    print(f"[run-pf] Running Pocketflow for {code_root} -> {output_dir}")
    try:
        completed = subprocess.run(
            cmd, cwd=REPO_ROOT, text=True, check=True
        )
        if completed.stdout:
            print(completed.stdout)
        if completed.stderr:
            print(completed.stderr)
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f"Pocketflow failed with exit code {exc.returncode}") from exc

    index_path = output_dir / project_name / "index.md"
    if not index_path.exists():
        raise SystemExit(f"Pocketflow output index.md not found at {index_path}")
    return index_path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Run Pocketflow for a single tool and update its MDX doc."
    )
    parser.add_argument("--tool", required=True, help="Tool name in tool-doc-map.json")
    parser.add_argument(
        "--language",
        default="english",
        help="Language for generated docs (default: english)",
    )
    parser.add_argument(
        "--no-cache",
        action="store_true",
        help="Disable Pocketflow LLM response caching",
    )
    parser.add_argument(
        "--max-abstractions",
        type=int,
        default=10,
        help="Maximum abstractions Pocketflow should identify (default: 10)",
    )
    parser.add_argument(
        "--skip-format",
        action="store_true",
        help="Skip running Prettier on the updated MDX file.",
    )
    parser.add_argument(
        "--single-file",
        action="store_true",
        help="Generate a single consolidated markdown file instead of index + separate chapter files (default: False, uses multi-chapter mode)",
    )
    args = parser.parse_args()

    tool_map = load_tool_map()
    entry = find_tool_entry(args.tool, tool_map)

    code_rel = entry.get("code_root")
    if not code_rel:
        raise SystemExit(f"'code_root' missing for tool {args.tool}")
    code_root = REPO_ROOT / code_rel
    if not code_root.exists():
        raise SystemExit(f"code_root path does not exist: {code_root}")

    # Run Pocketflow and get the path to index.md for this tool
    out_dir = OUTPUT_BASE / args.tool
    index_md = run_pocketflow_for_dir(
        code_root=code_root,
        output_dir=out_dir,
        language=args.language,
        use_cache=not args.no_cache,
        max_abstractions=args.max_abstractions,
        single_file=args.single_file,
    )

    body = index_md.read_text(encoding="utf-8")

    # Write body to a temporary file for the updater script
    tmp_body_path = out_dir / f"{args.tool}_body.md"
    out_dir.mkdir(parents=True, exist_ok=True)
    tmp_body_path.write_text(body, encoding="utf-8")

    # Call the MDX updater script
    updater = POCKETFLOW_DIR / "scripts" / "update_mdx_section.py"
    if not updater.exists():
        raise SystemExit(f"Updater script not found at {updater}")

    cmd = [
        sys.executable,
        str(updater),
        "--tool",
        args.tool,
        "--body-file",
        str(tmp_body_path),
    ]
    if args.skip_format:
        cmd.append("--skip-format")

    print(f"[run-pf] Updating MDX for tool {args.tool}")
    try:
        completed = subprocess.run(
            cmd, cwd=REPO_ROOT, text=True, check=True
        )
        if completed.stdout:
            print(completed.stdout)
        if completed.stderr:
            print(completed.stderr)
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f"Updater failed with exit code {exc.returncode}") from exc


if __name__ == "__main__":
    main()


