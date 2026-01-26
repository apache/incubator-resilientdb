#!/bin/bash
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

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

