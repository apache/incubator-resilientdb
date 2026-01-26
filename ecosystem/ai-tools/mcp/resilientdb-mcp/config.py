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

"""Configuration management for ResilientDB MCP Server."""
import os
from typing import Optional
from dotenv import load_dotenv

load_dotenv()


class Config:
    """Configuration class for ResilientDB MCP Server."""
    
    # GraphQL endpoint (port 8000 - for asset transactions only)
    GRAPHQL_URL: str = os.getenv(
        "RESILIENTDB_GRAPHQL_URL", 
        "http://localhost:8000/graphql"
    )
    
    # HTTP/Crow endpoint (port 18000 - for KV operations)
    HTTP_URL: str = os.getenv(
        "RESILIENTDB_HTTP_URL",
        "http://localhost:18000"
    )
    
    # Optional authentication
    API_KEY: Optional[str] = os.getenv("RESILIENTDB_API_KEY")
    AUTH_TOKEN: Optional[str] = os.getenv("RESILIENTDB_AUTH_TOKEN")
    
    # Timeout settings
    REQUEST_TIMEOUT: int = int(os.getenv("REQUEST_TIMEOUT", "30"))
    TRANSACTION_POLL_INTERVAL: float = float(os.getenv("TRANSACTION_POLL_INTERVAL", "1.0"))
    MAX_POLL_ATTEMPTS: int = int(os.getenv("MAX_POLL_ATTEMPTS", "30"))