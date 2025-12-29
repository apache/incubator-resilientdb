#!/bin/bash
# Initialize PostgreSQL in the combined container

set -e

# Initialize PostgreSQL if data directory is empty
if [ ! -f /var/lib/postgresql/data/PG_VERSION ]; then
    echo "Initializing PostgreSQL database..."
    su - postgres -c "/usr/lib/postgresql/14/bin/initdb -D /var/lib/postgresql/data"
    
    # Start PostgreSQL temporarily to create database and user
    su - postgres -c "/usr/lib/postgresql/14/bin/pg_ctl -D /var/lib/postgresql/data -l /var/lib/postgresql/data/logfile start"
    
    # Wait for PostgreSQL to be ready
    sleep 5
    
    # Create database and user
    su - postgres -c "psql -c \"CREATE USER ${POSTGRES_USER:-nexus} WITH PASSWORD '${POSTGRES_PASSWORD:-nexus}';\""
    su - postgres -c "psql -c \"CREATE DATABASE ${POSTGRES_DB:-nexus} OWNER ${POSTGRES_USER:-nexus};\""
    su - postgres -c "psql -d ${POSTGRES_DB:-nexus} -c \"CREATE EXTENSION IF NOT EXISTS vector;\""
    
    # Stop PostgreSQL (supervisor will start it)
    su - postgres -c "/usr/lib/postgresql/14/bin/pg_ctl -D /var/lib/postgresql/data stop"
    
    echo "PostgreSQL initialized successfully"
else
    echo "PostgreSQL data directory already exists, skipping initialization"
fi

