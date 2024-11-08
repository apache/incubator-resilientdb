from dataclasses import dataclass
from typing import Optional


@dataclass
class MongoConfig:
    uri: str
    db_name: str
    collection_name: str


@dataclass
class ResilientDBConfig:
    base_url: str  # e.g., 'resilientdb://localhost:18000'
    http_secure: bool = False
    ws_secure: bool = False
    http_endpoint: Optional[str] = None
    ws_endpoint: Optional[str] = None
    reconnect_interval: int = 5000  # in milliseconds
    fetch_interval: int = 30000  # in milliseconds