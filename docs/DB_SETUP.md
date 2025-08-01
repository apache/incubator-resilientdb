# PostgreSQL + Vector Storage Setup

## Prerequisites
- PostgreSQL with pgvector extension
- Node.js packages: pg, pgvector, @llamaindex/postgres

## Setup
```bash
# Install PostgreSQL & pgvector
brew install postgresql pgvector
brew services start postgresql

# Create database  
createdb nexus_db

# Setup vector storage
npm run db:setup-vector
```

## Architecture
- `document_indices`: Document parsing cache
- `document_embeddings`: Vector embeddings for AI search