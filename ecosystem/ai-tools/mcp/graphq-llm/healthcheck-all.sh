#!/bin/sh
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

