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
```

## Add env variables:
DATABASE_URL=postgresql://{username}@localhost:5432/nexus_db

