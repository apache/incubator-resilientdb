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

"""GraphQL client for ResilientDB operations."""
import httpx
from typing import Dict, Any, Optional
from config import Config
import json


class GraphQLClient:
    """Client for executing GraphQL queries and mutations on ResilientDB."""
    
    def __init__(self, url: str = None, api_key: Optional[str] = None):
        self.url = url or Config.GRAPHQL_URL
        self.http_url = Config.HTTP_URL  # HTTP/Crow server for KV operations
        self.api_key = api_key or Config.API_KEY
        self.timeout = Config.REQUEST_TIMEOUT
        
    def _get_headers(self) -> Dict[str, str]:
        """Get HTTP headers for requests."""
        headers = {
            "Content-Type": "application/json",
        }
        if self.api_key:
            headers["Authorization"] = f"Bearer {self.api_key}"
        return headers
    
    async def execute_query(self, query: str, variables: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """
        Execute a GraphQL query.
        
        Args:
            query: GraphQL query string
            variables: Optional variables for the query
            
        Returns:
            Response data from GraphQL server
        """
        payload = {
            "query": query,
            "variables": variables or {}
        }
        
        async with httpx.AsyncClient(timeout=self.timeout) as client:
            response = await client.post(
                self.url,
                json=payload,
                headers=self._get_headers()
            )
            response.raise_for_status()
            result = response.json()
            
            if "errors" in result:
                error_msg = json.dumps(result["errors"], indent=2)
                raise Exception(f"GraphQL errors: {error_msg}")
            
            return result.get("data", {})
    
    async def create_account(self, account_id: Optional[str] = None) -> Dict[str, Any]:
        """
        Create a new account in ResilientDB.
        Account creation requires generating keys outside of GraphQL.
        """
        raise Exception(
            "createAccount is not available. "
            "Account creation requires generating cryptographic keys outside of GraphQL. "
            "Use external key generation tools to create accounts."
        )
    
    async def get_transaction(self, transaction_id: str) -> Dict[str, Any]:
        """
        Get transaction by ID via GraphQL.
        Returns RetrieveTransaction with all available fields.
        
        Based on official documentation: https://beacon.resilientdb.com/docs/resilientdb_graphql#get-transaction-by-id
        
        Note: All fields except 'metadata' are NON_NULL and must be included in the query.
        """
        query = """
        query GetTx($id: ID!) {
            getTransaction(id: $id) {
                id
                version
                amount
                uri
                type
                publicKey
                operation
                metadata
                asset
                signerPublicKey
            }
        }
        """
        return await self.execute_query(query, {"id": transaction_id})
    
    async def post_transaction(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Post a new asset transaction via GraphQL.
        Requires PrepareAsset with all required fields:
        - operation (String): Transaction operation type
        - amount (Int): Transaction amount
        - signerPublicKey (String): Public key of the signer
        - signerPrivateKey (String): Private key of the signer
        - recipientPublicKey (String): Public key of the recipient
        - asset (JSONScalar): Asset data as JSON object with 'data' field (not string!)
        
        Based on official documentation: https://beacon.resilientdb.com/docs/resilientdb_graphql#get-transaction-by-id
        
        The asset must be structured as: {"data": {...}} where {...} contains your actual asset data.
        
        Returns CommitTransaction with transaction ID.
        """
        # Validate required fields
        required_fields = ["operation", "amount", "signerPublicKey", "signerPrivateKey", "recipientPublicKey", "asset"]
        missing_fields = [field for field in required_fields if field not in data]
        if missing_fields:
            raise Exception(
                f"Missing required fields in PrepareAsset: {', '.join(missing_fields)}. "
                f"Required fields: {', '.join(required_fields)}"
            )
        
        # IMPORTANT: Keep asset as dict/object - JSONScalar expects JSON object, not string
        # MCP framework may convert the asset object to a string, so we need to parse it back
        asset = data["asset"]
        if isinstance(asset, str):
            try:
                asset = json.loads(asset)
            except json.JSONDecodeError as e:
                raise Exception(
                    f"Failed to parse asset JSON string: {e}. "
                    f"Asset value: {asset[:100]}..."
                )
        
        # Ensure asset is a dict/object (not a list, string, or primitive)
        if not isinstance(asset, dict):
            raise Exception(
                f"Asset must be a JSON object (dict), but got {type(asset).__name__}. "
                f"Value: {str(asset)[:100]}..."
            )
        
        # Update data with parsed asset
        data["asset"] = asset
        
        mutation = """
        mutation Test($data: PrepareAsset!) {
            postTransaction(data: $data) {
                id
            }
        }
        """
        return await self.execute_query(mutation, {"data": data})
    
    async def update_transaction(self, transaction_id: str, data: Dict[str, Any]) -> Dict[str, Any]:
        """
        Update an existing transaction.
        Blockchain transactions are immutable once committed.
        """
        raise Exception(
            "updateTransaction is not available. "
            "Blockchain transactions are immutable once committed. "
            "To modify data, create a new transaction instead."
        )
    
    async def get_key_value(self, key: str) -> Dict[str, Any]:
        """
        Query key-value store via HTTP REST API (Crow server on port 18000).
        """
        async with httpx.AsyncClient(timeout=self.timeout) as client:
            response = await client.get(
                f"{self.http_url}/v1/transactions/{key}",
                headers=self._get_headers()
            )
            response.raise_for_status()
            result = response.json()
            
            # Return in a consistent format
            return {
                "key": key,
                "value": result.get("value", result),
                "response": result
            }
    
    async def set_key_value(self, key: str, value: Any) -> Dict[str, Any]:
        """
        Set key-value pair via HTTP REST API (Crow server on port 18000).
        """
        # Convert value to string if it's not already
        if not isinstance(value, str):
            value = json.dumps(value)
        
        async with httpx.AsyncClient(timeout=self.timeout) as client:
            response = await client.post(
                f"{self.http_url}/v1/transactions/commit",
                json={"id": key, "value": value},
                headers=self._get_headers()
            )
            response.raise_for_status()
            
            # Parse response text (Crow returns plain text like "id: key")
            response_text = response.text.strip()
            
            return {
                "key": key,
                "value": value,
                "status": "committed",
                "response": response_text
            }