# Automated Documentation Workflow

This document explains how the automated documentation generation workflow works for the ResilientDB ecosystem tools.

## Overview

The automated workflow uses Pocketflow to generate documentation for ecosystem tools whenever their code changes. The documentation is automatically updated in Beacon (the documentation site) and a pull request is created with the changes.

## How It Works

### 1. Trigger

The workflow (`.github/workflows/auto-docs.yml`) is triggered when:
- **Push events**: Code is pushed to `main` or `master` branch with changes in `ecosystem/**`
- **Manual trigger**: Can be manually triggered via GitHub Actions UI (`workflow_dispatch`)

### 2. Detection Phase

The workflow runs `scripts/detect_changed_tools.py` which:
- Compares the current commit with the previous commit (`HEAD^`)
- Checks which files changed in the `ecosystem/` directory
- Matches changed files against tool code roots defined in `tool-doc-map.json`
- Returns a list of tools that have code changes

**Example**: If you modify files in `ecosystem/tools/rescli/`, it will detect `rescli` as a changed tool.

### 3. Documentation Generation

For each detected tool:
1. Runs Pocketflow against the tool's code root directory
2. Generates documentation using the configured LLM (Gemini)
3. Updates the corresponding MDX file in `ecosystem/AI-Tooling/beacon/content/`
4. The MDX file is updated by replacing the auto-generated section with new content

### 4. Pull Request Creation

If documentation changes are detected:
- All updated MDX files are staged
- A commit is created with message: `"docs: auto-update documentation [skip docs]"`
- A pull request is automatically created with:
  - Title: `"docs: Auto-update documentation"`
  - Branch: `auto-docs-update`
  - Labels: `documentation`, `automated`
  - Body listing all changed tools

## Skipping Documentation Generation

To skip documentation generation for a specific commit, include `[skip docs]` in your commit message:

```bash
git commit -m "fix: update rescli command [skip docs]"
```

**Important Notes:**
- The `[skip docs]` tag only works for **push events** (not manual triggers)
- The workflow will still run but will exit early if it detects `[skip docs]` in the commit message
- The auto-generated commit messages already include `[skip docs]` to prevent infinite loops

## Configuration

### Tool Mapping: `tool-doc-map.json`

This is the **main configuration file** where you map tools to their code locations and documentation paths.

**Location**: `ecosystem/pocketflow/tool-doc-map.json`

**Format**:
```json
[
  {
    "name": "tool-name",
    "code_root": "ecosystem/path/to/tool",
    "doc_path": "ecosystem/AI-Tooling/beacon/content/tool-name.mdx"
  }
]
```

**Fields**:
- `name`: Unique identifier for the tool (used in scripts and commands)
- `code_root`: Path to the tool's source code directory (relative to repo root)
- `doc_path`: Path to the MDX file in Beacon where documentation will be updated

### Adding a New Tool

To add documentation generation for a new tool:

1. **Add entry to `tool-doc-map.json`**:
   ```json
   {
     "name": "my-new-tool",
     "code_root": "ecosystem/tools/my-new-tool",
     "doc_path": "ecosystem/AI-Tooling/beacon/content/my-new-tool.mdx"
   }
   ```

2. **Create the MDX file** (if it doesn't exist):
   - Location: `ecosystem/AI-Tooling/beacon/content/my-new-tool.mdx`
   - Must include the auto-doc section marker (see below)

3. **Ensure the MDX file has the auto-doc markers**:
   The MDX file must include the auto-doc section markers. The script will create them automatically if the file doesn't exist, but if you're creating it manually, use this format:
   ```mdx
   # My New Tool
   
   {{/* BEGIN AUTO_DOC: my-new-tool */}}
   
   <!-- Pocketflow will overwrite this block -->
   
   {{/* END AUTO_DOC: my-new-tool */}}
   ```
   
   **Important**: The tool name in the markers must match the `name` field in `tool-doc-map.json`.

### Excluding Paths from Triggering Updates

The `detect_changed_tools.py` script automatically excludes:
- `ecosystem/AI-Tooling/beacon/**` - Don't trigger on doc changes
- `ecosystem/pocketflow/**` - Don't trigger on Pocketflow changes

These are defined in the `EXCLUDE_PATTERNS` list in `scripts/detect_changed_tools.py`.

## Environment Variables

The workflow requires these GitHub Secrets to be configured:

- `GEMINI_API_KEY`: Your Google Gemini API key
- `GEMINI_MODEL`: (Optional) Model name, defaults to `models/gemini-2.5-pro`

These are used by Pocketflow to generate the documentation.

## Scripts Reference

### `scripts/detect_changed_tools.py`

Detects which tools have changed by comparing git commits.

**Usage**:
```bash
python3 scripts/detect_changed_tools.py [--base-ref <ref>]
```

**Output**: One tool name per line (e.g., `rescli\nresvault\nreslens`)

**Options**:
- `--base-ref`: Git reference to compare against (default: `HEAD^`)

### `scripts/run_pocketflow_for_tool.py`

Runs Pocketflow for a single tool and updates its MDX documentation.

**Usage**:
```bash
python3 scripts/run_pocketflow_for_tool.py \
  --tool <tool-name> \
  [--single-file] \
  [--skip-format] \
  [--no-cache] \
  [--max-abstractions <num>]
```

**Options**:
- `--tool`: Tool name from `tool-doc-map.json` (required)
- `--single-file`: Generate single consolidated file instead of chapters
- `--skip-format`: Skip running Prettier on updated MDX
- `--no-cache`: Disable LLM response caching
- `--max-abstractions`: Maximum abstractions to identify (default: 10)

### `scripts/update_mdx_section.py`

Updates the auto-generated section in an MDX file with new content.

**Usage**:
```bash
python3 scripts/update_mdx_section.py \
  --tool <tool-name> \
  --body-file <path-to-generated-md> \
  [--skip-format]
```

This is called automatically by `run_pocketflow_for_tool.py`.

## Testing Locally

You can test the workflow locally:

1. **Test detection**:
   ```bash
   cd ecosystem/pocketflow
   python3 scripts/detect_changed_tools.py --base-ref HEAD~1
   ```

2. **Test documentation generation for a single tool**:
   ```bash
   cd ecosystem/pocketflow
   export GEMINI_API_KEY="your-key"
   export GEMINI_MODEL="models/gemini-2.5-pro"
   export LLM_PROVIDER="GEMINI"
   
   python3 scripts/run_pocketflow_for_tool.py \
     --tool rescontract \
     --single-file \
     --skip-format
   ```

## Troubleshooting

### Workflow fails with "Invalid format" error

This usually means multiple tools were detected but the output format was incorrect. This has been fixed in the workflow, but if you see this:
- Check that `tool-doc-map.json` has valid JSON
- Ensure tool names don't contain spaces or special characters

### Documentation not updating

- Check that the MDX file exists at the path specified in `tool-doc-map.json`
- Verify the MDX file has the correct auto-doc section markers
- Check GitHub Actions logs for specific errors

### Tool not detected when code changes

- Verify the `code_root` path in `tool-doc-map.json` matches the actual directory structure
- Check that the changed files are actually under the `code_root` path
- Ensure the files aren't in excluded paths (beacon, pocketflow)

## Future Enhancements

Potential improvements:
- Support for multiple documentation formats
- Customizable LLM prompts per tool
- Parallel documentation generation for multiple tools
- Support for partial updates (only changed sections)

## Related Files

- Workflow: `.github/workflows/auto-docs.yml`
- Tool mapping: `ecosystem/pocketflow/tool-doc-map.json`
- Detection script: `ecosystem/pocketflow/scripts/detect_changed_tools.py`
- Generation script: `ecosystem/pocketflow/scripts/run_pocketflow_for_tool.py`
- MDX updater: `ecosystem/pocketflow/scripts/update_mdx_section.py`
