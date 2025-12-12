#!/bin/sh
# Health check for all-in-one container (PostgreSQL + GraphQ-LLM)
# Note: ResilientDB runs in a separate container but same network

# Check PostgreSQL
pg_isready -U ${POSTGRES_USER:-nexus} -d ${POSTGRES_DB:-nexus} > /dev/null 2>&1
PG_STATUS=$?

# Check GraphQ-LLM Backend
curl -f http://localhost:3001/health > /dev/null 2>&1
LLM_STATUS=$?

# Both must be healthy (ResilientDB is checked separately)
if [ $PG_STATUS -eq 0 ] && [ $LLM_STATUS -eq 0 ]; then
    exit 0
else
    exit 1
fi

