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

# ResInsight MCP — container image
#
# Build:  docker build -t resinsight-mcp .
# Run:    docker run --rm -p 8005:8005 --env-file .env resinsight-mcp
#
# Required env (see README): GITHUB_TOKEN, MCP_TOKEN
# Listen on all interfaces inside the container:
#   FASTMCP_HOST=0.0.0.0

FROM python:3.11-slim-bookworm

ENV PYTHONUNBUFFERED=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    MPLBACKEND=Agg \
    FASTMCP_HOST=0.0.0.0

WORKDIR /app

# faiss / numpy wheels often need OpenMP at runtime
RUN apt-get update \
    && apt-get install -y --no-install-recommends libgomp1 \
    && rm -rf /var/lib/apt/lists/*

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

EXPOSE 8005

CMD ["python", "server.py"]
