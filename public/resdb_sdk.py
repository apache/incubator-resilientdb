import json
import js
import asyncio
from typing import Dict, Any, Optional, List

RESDB_REST_URL = "https://crow.resilientdb.com"

class EndpointError(Exception):
    """Custom exception for endpoint-related errors"""
    pass

class TransactionDriver:
    def __init__(self, url=RESDB_REST_URL):
        self.url = url

    async def check_health(self) -> bool:
        """Check if the endpoint is healthy"""
        try:
            response = await js.fetch(f"{self.url}/health")
            return response.status == 200
        except Exception as e:
            js.js_print(f"Health check failed: {str(e)}")
            return False

    async def send_commit(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Send a transaction to be committed"""
        try:
            # Ensure data matches exact format that works
            if not isinstance(data, dict) or "id" not in data or "value" not in data:
                raise ValueError("Transaction must be a dictionary with 'id' and 'value' fields")
            
            # Ensure data types are correct
            transaction = {
                "id": str(data["id"]),
                "value": str(data["value"])
            }
            
            # Add proper headers exactly like Postman
            headers = {
                "Content-Type": "application/json",
                "Accept": "*/*",  # Changed to match Postman's Accept header
                "User-Agent": "PostmanRuntime/7.36.0",  # Match Postman's User-Agent
                "Accept-Encoding": "gzip, deflate, br",
                "Connection": "keep-alive"
            }
            
            # Convert headers to a format js.fetch can understand
            js_headers = {}
            for key, value in headers.items():
                js_headers[key] = value
            
            print("Sending request with:")
            print(f"Headers: {json.dumps(headers, indent=2)}")
            print(f"Body: {json.dumps(transaction, indent=2)}")
            
            response = await js.fetch(f"{self.url}/v1/transactions/commit", {
                "method": "POST",
                "headers": js_headers,
                "body": json.dumps(transaction)
            })
            
            print(f"Response status: {response.status}")
            print(f"Response headers: {dict(response.headers)}")
            
            # Handle both 200 and 201 status codes
            if response.status in [200, 201]:
                try:
                    response_text = await response.text()
                    print(f"Raw response: {response_text}")
                    if response_text:
                        return json.loads(response_text)
                    return {"status": response.status, "success": True, "message": "Transaction created" if response.status == 201 else "Transaction processed"}
                except Exception as e:
                    print(f"Failed to parse response: {str(e)}")
                    return {"status": response.status, "success": True}
            else:
                response_text = await response.text()
                print(f"Error response: {response_text}")
                return {"status": response.status, "success": False, "error": response_text}
                
        except Exception as e:
            print(f"Failed to commit transaction: {str(e)}")
            raise Exception(str(e))

    async def retrieve(self, key: str) -> Dict[str, Any]:
        """Retrieve a transaction by key"""
        try:
            response = await js.fetch(f"{self.url}/v1/transactions/{key}")
            response_text = await response.text()
            try:
                return json.loads(response_text) if response_text else {}
            except:
                return {}
        except Exception as e:
            print(f"Failed to retrieve transaction: {str(e)}")
            raise Exception(str(e))

    async def get_unspent_outputs(self, public_key: str) -> List[Dict[str, Any]]:
        """Get unspent transaction outputs for a public key"""
        try:
            response = await js.fetch(f"{self.url}/v1/transactions/unspent/{public_key}")
            result = await response.json()
            js.js_print(f"Unspent outputs: {json.dumps(result, indent=2)}")
            return result
        except Exception as e:
            error_msg = f"Failed to get unspent outputs: {str(e)}"
            js.js_print(error_msg)
            raise EndpointError(error_msg)

class Resdb:
    def __init__(self, url=RESDB_REST_URL):
        self.url = url
        self.transactions = TransactionDriver(url)

    async def check_connection(self) -> bool:
        """Check if we can connect to ResilientDB"""
        return await self.transactions.check_health()

    def get_example_code(self, example_type: str) -> str:
        """Get example code for different ResilientDB operations"""
        examples = {
            "simple_transaction": '''
# Simple transaction example
transaction_data = {
    "sender": "sender_public_key",
    "recipient": "recipient_public_key",
    "amount": 100,
    "metadata": {"description": "Test transaction"}
}

# Send the transaction
result = await db.transactions.send_commit(transaction_data)
print(f"Transaction ID: {result.get('id')}")
''',
            "retrieve_transaction": '''
# Retrieve a transaction by ID
transaction_id = "your_transaction_id"
transaction = await db.transactions.retrieve(transaction_id)
print(f"Transaction details: {transaction}")
''',
            "check_unspent": '''
# Check unspent outputs for a public key
public_key = "your_public_key"
unspent = await db.transactions.get_unspent_outputs(public_key)
print(f"Unspent outputs: {unspent}")
''',
            "health_check": '''
# Check if ResilientDB endpoint is healthy
is_healthy = await db.check_connection()
print(f"Endpoint is {'healthy' if is_healthy else 'unhealthy'}")
'''
        }
        return examples.get(example_type, "Example type not found")

db = Resdb()

# Add example code as comments for easy access
EXAMPLE_CODE = """
# Example 1: Simple Transaction
transaction_data = {
    "sender": "sender_public_key",
    "recipient": "recipient_public_key",
    "amount": 100,
    "metadata": {"description": "Test transaction"}
}
result = await db.transactions.send_commit(transaction_data)

# Example 2: Retrieve Transaction
transaction = await db.transactions.retrieve("transaction_id")

# Example 3: Check Unspent Outputs
unspent = await db.transactions.get_unspent_outputs("public_key")

# Example 4: Health Check
is_healthy = await db.check_connection()
"""

class Client:
    def __init__(self, url="https://crow.resilientdb.com"):
        self.url = url.rstrip("/")
    
    def create_transaction(self, id, value):
        """Create a transaction object with id and value."""
        return {
            "id": str(id),
            "value": str(value)
        }
    
    async def send_transaction(self, transaction):
        """Send a transaction to the /v1/transactions/commit endpoint."""
        try:
            # Ensure proper headers and request format
            headers = {
                "Content-Type": "application/json",
                "Accept": "application/json",
                "User-Agent": "ResilientDB-Python-SDK/1.0",
                "Accept-Encoding": "gzip, deflate, br",
                "Connection": "keep-alive"
            }
            
            # Ensure transaction data is properly formatted
            if not isinstance(transaction, dict):
                raise ValueError("Transaction must be a dictionary")
            if "id" not in transaction or "value" not in transaction:
                raise ValueError("Transaction must contain 'id' and 'value' fields")
                
            # Make request exactly like Postman
            response = await js.fetch(f"{self.url}/v1/transactions/commit", {
                "method": "POST",
                "headers": js.Object.fromEntries(list(headers.items())),
                "body": json.dumps(transaction)
            })
            
            # Handle response
            if response.status == 200:
                try:
                    response_text = await response.text()
                    if response_text:
                        return json.loads(response_text)
                    return {"status": response.status, "success": True}
                except:
                    return {"status": response.status, "success": True}
            else:
                return {"status": response.status, "success": False, "error": "Transaction failed"}
            
        except Exception as e:
            print(f"Error sending transaction: {str(e)}")
            raise Exception(str(e))
    
    async def get_transaction(self, transaction_id):
        """Get a transaction by its ID."""
        try:
            response = await js.fetch(f"{self.url}/v1/transactions/{transaction_id}")
            response_text = await response.text()
            try:
                return json.loads(response_text) if response_text else {}
            except:
                return response_text if response_text else {}
        except Exception as e:
            print(f"Error getting transaction: {str(e)}")
            raise Exception(str(e))
