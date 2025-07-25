# PostgreSQL Setup & Migration Guide

## Environment Variables
```env
DATABASE_URL=postgresql://user@localhost:5432/nexus_db
```

## macOS Setup
```bash
# Install PostgreSQL
brew install postgresql
brew services start postgresql

# Create database
createdb nexus_db

# Test connection
psql nexus_db
```

## Vercel Setup
```bash
# Add PostgreSQL addon
vercel postgres create
```

## Migration
```bash
# Install
npm install

# Run migration
npm run db:migrate
```

## Verification
```bash
# Test connection
psql postgresql://aibrahi@localhost:5432/nexus_db
```

## Cleanup
```bash
rm -rf documents/parsed/
```

## Troubleshooting
```bash
# Connection issues
brew services restart postgresql
``` 