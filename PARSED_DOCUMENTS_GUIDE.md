# Parsed Documents Caching System

This system allows you to save and reuse previously parsed documents, avoiding the need to re-parse files from scratch using LlamaParse. This saves time, API costs, and provides better performance.

## How It Works

### 1. **Automatic Caching**
When a document is parsed for the first time using LlamaParse, the results are automatically saved to:
```
documents/parsed/{document-name}/
├── documents.json    # Parsed content and metadata
└── metadata.json     # Document metadata and cache info
```

### 2. **Smart Cache Loading**
Before parsing a document, the system checks:
- ✅ Do pre-parsed files exist?
- ✅ Are they newer than the source document?
- ✅ If yes, load from cache instead of re-parsing

### 3. **Fallback to Parsing**
If cache is missing, outdated, or corrupted, the system automatically falls back to LlamaParse.

## File Structure

```
documents/
├── resilientdb.pdf                    # Original PDF
└── parsed/
    └── resilientdb/
        ├── documents.json             # Parsed document chunks
        └── metadata.json              # Cache metadata
```

### documents.json Format
```json
[
  {
    "id_": "unique-document-id",
    "text": "Document content chunk...",
    "metadata": {}
  },
  // ... more document chunks
]
```

### metadata.json Format
```json
{
  "documentPath": "documents/resilientdb.pdf",
  "originalFileSize": 1145477,
  "originalModifiedTime": "2025-06-01T03:32:47.784Z",
  "cachedAt": "2025-06-01T16:14:01.565Z",
  "documentsCount": 16
}
```

## Using Pre-parsed Documents

### Method 1: Automatic (Recommended)
Simply use the API as normal. The system automatically:
1. Checks for valid cached files
2. Loads from cache if available and up-to-date
3. Falls back to LlamaParse if needed
4. Saves results for future use

### Method 2: Manual Pre-parsing
You can manually add parsed files by:

1. **Creating the directory structure:**
```bash
mkdir -p documents/parsed/{document-name}
```

2. **Adding documents.json:**
Place your parsed content in the LlamaIndex Document format

3. **Adding metadata.json:**
Include proper metadata for cache validation

## Management Commands

Use the provided script to manage your parsed documents:

```bash
# Check cache status for all documents
node scripts/manage-parsed-docs.js status

# List all pre-parsed documents with details
node scripts/manage-parsed-docs.js list

# Clear all cache
node scripts/manage-parsed-docs.js clear

# Clear cache for specific document
node scripts/manage-parsed-docs.js clear resilientdb

# Show help
node scripts/manage-parsed-docs.js help
```

## Benefits

### 🚀 **Performance**
- **Instant loading** of pre-parsed documents
- No waiting for LlamaParse API calls
- Faster vector index creation

### 💰 **Cost Savings**
- Avoid repeated LlamaParse API calls
- Save on LlamaCloud credits
- Reduced bandwidth usage

### 🔄 **Reliability**
- Works offline for cached documents
- Fallback to parsing if cache fails
- Automatic cache validation

### 🎯 **Smart Caching**
- Only re-parses when source document changes
- Automatic cache invalidation
- Preserves original document structure

## Example Workflow

### Initial Parse (First Time)
```typescript
// User asks question about documents/research-paper.pdf
// System checks: No cache found
// System parses with LlamaParse
// System saves to documents/parsed/research-paper/
// System creates vector index and responds
```

### Subsequent Requests (Cached)
```typescript
// User asks question about documents/research-paper.pdf
// System checks: Cache found and up-to-date
// System loads from documents/parsed/research-paper/documents.json
// System creates vector index and responds (much faster!)
```

### Updated Document
```typescript
// User replaces documents/research-paper.pdf with newer version
// System checks: Cache exists but is outdated
// System re-parses with LlamaParse
// System updates cache with new content
// System responds with updated content
```

## Best Practices

### 1. **Document Organization**
- Keep original documents in `documents/` folder
- Don't manually edit cached files (they'll be overwritten)
- Use descriptive filenames for better cache organization

### 2. **Cache Management**
- Periodically check cache status: `node scripts/manage-parsed-docs.js status`
- Clear outdated cache when needed
- Monitor disk space usage for large document collections

### 3. **Backup Strategy**
- Consider backing up the `documents/parsed/` directory
- Cached files can be regenerated but save time and API costs
- Include parsed cache in your deployment strategy

### 4. **Development Workflow**
```bash
# Before deploying
node scripts/manage-parsed-docs.js status

# Clear cache if needed
node scripts/manage-parsed-docs.js clear outdated-doc

# Deploy with fresh cache
```

## Troubleshooting

### Cache Not Loading
**Symptoms:** Document gets re-parsed every time
**Solutions:**
1. Check file permissions on `documents/parsed/`
2. Verify `documents.json` and `metadata.json` exist
3. Check if source document is newer than cache
4. Review console logs for detailed error messages

### Corrupted Cache
**Symptoms:** JSON parsing errors, missing content
**Solutions:**
1. Clear specific document cache: `node scripts/manage-parsed-docs.js clear doc-name`
2. Let system re-parse automatically
3. Check for disk space issues

### Performance Issues
**Symptoms:** Slow loading despite cached files
**Solutions:**
1. Check cache file sizes (very large files may be slow to load)
2. Consider splitting large documents
3. Monitor memory usage during vector index creation

## Technical Notes

### LlamaIndex Document Format
The system preserves the exact LlamaIndex `Document` format:
- `id_`: Unique identifier for each document chunk
- `text`: The actual content text
- `metadata`: Additional metadata (can be empty)

### File Naming Convention
- Document name is derived from filename (without extension)
- Special characters are preserved in directory names
- Case sensitivity follows filesystem conventions

### Cache Validation
The system uses file modification times (`mtime`) to determine if cache is valid:
- If source document is newer than cache → re-parse
- If cache is newer or same age → use cache
- If cache files are missing → re-parse

This ensures you always get the most up-to-date content while maximizing cache efficiency. 