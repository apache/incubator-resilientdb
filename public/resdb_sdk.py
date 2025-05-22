import json
import js
import asyncio
from typing import Dict, Any, Optional, List
import time
from pyodide.http import pyfetch, FetchResponse

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
            response = await pyfetch(f"{self.url}/health")
            return response.status == 200
        except Exception as e:
            js.js_print(f"Health check failed: {str(e)}")
            return False

    async def send_commit(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """Send a transaction to be committed"""
        try:
            # Keep only the essential fields and format
            transaction = {
                "id": str(data["id"]),
                "value": str(data["value"]),
                "type": "kv"
            }
            
            # Simplify headers to just the essential ones
            headers = {
                "Content-Type": "application/json",
                "Accept": "*/*"
            }
            
            print(f"\nSending POST to {self.url}/v1/transactions/commit")
            print(f"Headers: {json.dumps(headers, indent=2)}")
            print(f"Body: {json.dumps(transaction, indent=2)}")
            
            # Use pyfetch instead of js.fetch
            response = await pyfetch(
                f"{self.url}/v1/transactions/commit",
                method="POST",
                headers=headers,
                body=json.dumps(transaction)
            )
            
            print(f"\nResponse status: {response.status}")
            print(f"Response headers: {dict(response.headers)}")
            
            response_text = await response.text()
            print(f"Raw response text: '{response_text}'")
            
            # Handle 201 Created status with id: response
            if response.status == 201 and response_text.startswith('id:'):
                tx_id = response_text.split(':')[1].strip()
                return {
                    "status": 201,
                    "success": True,
                    "id": tx_id,
                    "message": "Transaction created successfully"
                }
            
            # Handle 200 OK status - must have non-empty response to be considered success
            if response.status == 200 and response_text:
                return {
                    "status": 200,
                    "success": True,
                    "id": transaction["id"],
                    "message": "Transaction accepted"
                }
            
            # Empty response with 200 status means transaction was not accepted
            if response.status == 200 and not response_text:
                return {
                    "status": 200,
                    "success": False,
                    "id": transaction["id"],
                    "message": "Transaction was not accepted - empty response"
                }
            
            # Handle other responses
            return {
                "status": response.status,
                "success": False,
                "error": response_text or "Unexpected response"
            }
                
        except Exception as e:
            print(f"Error in send_commit: {str(e)}")
            raise Exception(str(e))

    async def retrieve(self, tx_id: str) -> Dict[str, Any]:
        """Retrieve a transaction by tx_id"""
        try:
            # Match Postman headers exactly
            headers = {
                "Accept": "*/*",
                "Content-Type": "application/json"
            }
            
            print(f"\nSending GET to {self.url}/v1/transactions/{tx_id}")
            
            # Use pyfetch instead of js.fetch
            response = await pyfetch(
                f"{self.url}/v1/transactions/{tx_id}",
                method="GET",
                headers=headers
            )
            
            print(f"Response status: {response.status}")
            print(f"Response headers: {dict(response.headers)}")
            
            response_text = await response.text()
            print(f"Raw response text: '{response_text}'")
            
            if response.status == 200:
                # Empty response with 200 status means transaction doesn't exist
                if not response_text:
                    return {
                        "status": 404,
                        "success": False,
                        "id": tx_id,
                        "message": "Transaction not found (empty response)"
                    }
                
                try:
                    # Try to parse as JSON first
                    return json.loads(response_text)
                except json.JSONDecodeError:
                    # If not JSON, try to extract value from response
                    try:
                        # Return the transaction data in a structured format
                        return {
                            "status": 200,
                            "success": True,
                            "id": tx_id,
                            "value": response_text.strip(),
                            "type": "kv"
                        }
                    except:
                        return {
                            "status": 200,
                            "success": True,
                            "id": tx_id,
                            "raw_response": response_text
                        }
            elif response.status == 404:
                return {
                    "status": 404,
                    "success": False,
                    "message": "Transaction not found"
                }
            else:
                return {
                    "status": response.status,
                    "success": False,
                    "error": response_text or "Unknown error"
                }
                
        except Exception as e:
            print(f"Error in retrieve: {str(e)}")
            raise Exception(str(e))

    async def get_unspent_outputs(self, public_key: str) -> List[Dict[str, Any]]:
        """Get unspent transaction outputs for a public key"""
        try:
            response = await pyfetch(f"{self.url}/v1/transactions/unspent/{public_key}")
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
                "Accept": "text/plain"
            }
            
            # Ensure transaction data is properly formatted
            if not isinstance(transaction, dict):
                raise ValueError("Transaction must be a dictionary")
            if "id" not in transaction or "value" not in transaction:
                raise ValueError("Transaction must contain 'id' and 'value' fields")
                
            # Make request exactly like curl
            response = await pyfetch(
                f"{self.url}/v1/transactions/commit",
                method="POST",
                headers=headers,
                body=json.dumps(transaction)
            )
            
            # Handle response
            response_text = await response.text()
            
            # Handle 201 Created with id: response
            if response.status == 201 and response_text.startswith('id:'):
                tx_id = response_text.split(':')[1].strip()
                return {
                    "status": 201,
                    "success": True,
                    "id": tx_id,
                    "message": "Transaction created successfully"
                }
            
            # Handle 200 OK status
            if response.status == 200:
                return {
                    "status": 200,
                    "success": True,
                    "id": transaction["id"],
                    "message": "Transaction accepted"
                }
            
            return {
                "status": response.status,
                "success": False,
                "error": "Transaction failed"
            }
            
        except Exception as e:
            print(f"Error sending transaction: {str(e)}")
            raise Exception(str(e))
    
    async def get_transaction(self, transaction_id):
        """Get a transaction by its ID."""
        try:
            headers = {
                "Accept": "application/json"
            }
            
            response = await pyfetch(
                f"{self.url}/v1/transactions/{transaction_id}",
                method="GET",
                headers=headers
            )
            
            response_text = await response.text()
            
            if response.status == 200:
                if response_text:
                    try:
                        return json.loads(response_text)
                    except:
                        return {
                            "status": 200,
                            "raw_response": response_text
                        }
                return {
                    "status": 200,
                    "success": True,
                    "id": transaction_id,
                    "message": "Transaction exists"
                }
            
            return {
                "status": response.status,
                "success": False,
                "message": "Transaction not found"
            }
            
        except Exception as e:
            print(f"Error getting transaction: {str(e)}")
            raise Exception(str(e))
