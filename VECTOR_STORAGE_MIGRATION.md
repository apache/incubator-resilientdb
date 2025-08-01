# PostgreSQL Vector Storage Migration Guide

This guide covers the migration from file-based vector storage to PostgreSQL with pgvector for LlamaIndex.TS in the Nexus project.

## Overview

The migration moves vector storage from LlamaIndex's default file-based storage to PostgreSQL using the `pgvector` extension. This provides:

- **Scalability**: Better performance with large datasets
- **Reliability**: ACID compliance and database robustness  
- **Production Ready**: Connection pooling, backups, monitoring
- **Multi-tenancy**: Collection support for data segregation

## Prerequisites

1. **PostgreSQL with pgvector**: Ensure your PostgreSQL instance has the `pgvector` extension available
2. **Database Access**: The same PostgreSQL database used for document caching
3. **Environment Variables**: Proper configuration in your `.env` file

## Migration Steps

### 1. Install Required Package

The `@llamaindex/postgres` package has been added to support PGVectorStore:

```bash
npm install @llamaindex/postgres
```

### 2. Set up Vector Storage in Database

Run the setup script to create the necessary tables and indexes:

```bash
npm run db:setup-vector
```

This script will:
- Enable the `pgvector` extension
- Create the vector embeddings table
- Set up HNSW indexes for optimal performance
- Create metadata and timestamp indexes
- Verify the setup

### 3. Environment Configuration

The following environment variables can be used to customize vector storage (all optional):

```env
# Vector Storage Configuration (optional - defaults to DATABASE_URL parsing)
POSTGRES_DATABASE=your_database_name
POSTGRES_HOST=localhost
POSTGRES_PORT=5432
POSTGRES_USER=your_username
POSTGRES_PASSWORD=your_password

# Vector Storage Settings
VECTOR_TABLE_NAME=document_embeddings  # Default table name
EMBED_DIM=1536                        # Embedding dimension (adjust for your model)

# HNSW Performance Tuning
HNSW_M=16                            # Number of bi-directional links
HNSW_EF_CONSTRUCTION=64              # Size of dynamic candidate list during construction
HNSW_EF_SEARCH=40                    # Size of dynamic candidate list during search
HNSW_DIST_METHOD=vector_cosine_ops   # Distance method for HNSW
```

### 4. Code Changes Made

#### New Files Created:
- `src/lib/vector-store.ts` - Vector store service for managing PGVectorStore
- `scripts/setup-vector-storage.ts` - Database setup script
- `VECTOR_STORAGE_MIGRATION.md` - This migration guide

#### Files Updated:
- `src/config/environment.ts` - Added vector storage configuration
- `src/lib/document-index-manager.ts` - Updated to use PGVectorStore
- `src/lib/query-engine.ts` - Added vector store imports
- `package.json` - Added setup script and postgres package

### 5. Verification

After migration, verify the setup:

1. **Check Database**: Ensure the vector table exists and has proper indexes
2. **Test Document Indexing**: Try indexing a document to verify vector storage works
3. **Test Queries**: Perform queries to ensure retrieval works correctly

```bash
# Check if pgvector extension is enabled
psql -d your_database -c "SELECT * FROM pg_extension WHERE extname = 'vector';"

# Check if the embeddings table exists
psql -d your_database -c "\\dt document_embeddings"

# Check indexes
psql -d your_database -c "\\di document_embeddings*"
```

## Architecture Changes

### Before Migration
```
Documents → LlamaParseReader → VectorStoreIndex (file storage) → Query Engine
                ↓
    PostgreSQL (document cache only)
```

### After Migration  
```
Documents → LlamaParseReader → VectorStoreIndex (PGVectorStore) → Query Engine
                ↓                        ↓
    PostgreSQL (document cache)  PostgreSQL (vector storage)
```

## Performance Considerations

1. **HNSW Parameters**: Tune `m`, `ef_construction`, and `ef_search` based on your data size and query patterns
2. **Embedding Dimensions**: Ensure `EMBED_DIM` matches your embedding model's output
3. **Connection Pooling**: PostgreSQL connection pooling is already configured
4. **Indexing Strategy**: The setup creates optimized indexes for vector similarity search

## Troubleshooting

### Common Issues

1. **pgvector Extension Not Found**
   ```bash
   # Install pgvector (Ubuntu/Debian)
   sudo apt-get install postgresql-15-pgvector
   
   # Or using Homebrew (macOS)
   brew install pgvector
   ```

2. **Permission Errors**
   - Ensure your database user has CREATE EXTENSION privileges
   - Check that the user can create tables and indexes

3. **Embedding Dimension Mismatch**
   - Verify `EMBED_DIM` matches your embedding model
   - Common dimensions: 1536 (OpenAI), 768 (BERT), 384 (MiniLM)

4. **Connection Issues**
   - Verify `DATABASE_URL` is correctly configured
   - Check PostgreSQL is running and accessible

### Health Check

Use the vector store service health check:

```typescript
import { vectorStoreService } from './src/lib/vector-store';

const isHealthy = await vectorStoreService.healthCheck();
console.log('Vector store healthy:', isHealthy);
```

## Rollback Plan

If you need to rollback to file-based storage:

1. Comment out the `storageContext` parameter in `DocumentIndexManager`
2. Remove the `vectorStoreService` imports
3. Restart the application

The original document cache in PostgreSQL will remain unaffected.

## Next Steps

1. **Test thoroughly** with your existing documents
2. **Monitor performance** and adjust HNSW parameters if needed  
3. **Set up backup strategy** for the vector embeddings table
4. **Consider collection-based multi-tenancy** if needed

## Support

For issues related to:
- **pgvector**: Check the [pgvector documentation](https://github.com/pgvector/pgvector)
- **LlamaIndex PGVectorStore**: Check the [LlamaIndex.TS documentation](https://ts.llamaindex.ai/)
- **PostgreSQL**: Check the [PostgreSQL documentation](https://www.postgresql.org/docs/)