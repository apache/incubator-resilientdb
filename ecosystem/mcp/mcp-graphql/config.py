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