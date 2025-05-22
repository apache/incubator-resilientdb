"""
ResilientDB Python SDK for Web Playground
This is a simplified version of the official ResilientDB Python SDK, adapted for web playground use.
"""
import json
import time
from typing import Dict, Any, Optional
from pyodide.http import pyfetch

RESDB_REST_URL = "https://crow.resilientdb.com"

class Transaction:
    def __init__(self, id: str, value: str):
        self.id = str(id)
        self.value = str(value)
        self.type = "kv"
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            "id": self.id,
            "value": self.value,
            "type": self.type
        }

class TransactionAPI:
    def __init__(self, url: str):
        self.url = url.rstrip("/")
    
    async def create(self, transaction: Transaction) -> Dict[str, Any]:
        """Create a new transaction"""
        try:
            headers = {
                "Content-Type": "application/json",
                "Accept": "*/*"
            }
            
            response = await pyfetch(
                f"{self.url}/v1/transactions/commit",
                method="POST",
                headers=headers,
                body=json.dumps(transaction.to_dict())
            )
            
            response_text = await response.text()
            
            if response.status == 201 and response_text.startswith('id:'):
                tx_id = response_text.split(':')[1].strip()
                return {
                    "status": 201,
                    "success": True,
                    "id": tx_id,
                    "message": "Transaction created successfully"
                }
            
            if response.status == 200:
                return {
                    "status": 200,
                    "success": True,
                    "id": transaction.id,
                    "message": "Transaction accepted"
                }
            
            return {
                "status": response.status,
                "success": False,
                "error": response_text or "Transaction failed"
            }
            
        except Exception as e:
            print(f"Error creating transaction: {str(e)}")
            raise Exception(str(e))

    async def retrieve(self, transaction_id: str) -> Dict[str, Any]:
        """Retrieve a transaction by ID"""
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
                            "success": True,
                            "id": transaction_id,
                            "value": response_text.strip(),
                            "type": "kv"
                        }
                return {
                    "status": 404,
                    "success": False,
                    "message": "Transaction not found (empty response)"
                }
            
            return {
                "status": response.status,
                "success": False,
                "message": "Transaction not found"
            }
            
        except Exception as e:
            print(f"Error retrieving transaction: {str(e)}")
            raise Exception(str(e))

class ResilientDB:
    def __init__(self, url: str = RESDB_REST_URL):
        """Initialize ResilientDB client"""
        self.url = url.rstrip("/")
        self.transactions = TransactionAPI(self.url)
    
    async def is_healthy(self) -> bool:
        """Check if the ResilientDB endpoint is healthy"""
        try:
            response = await pyfetch(f"{self.url}/health")
            return response.status == 200
        except Exception as e:
            print(f"Health check failed: {str(e)}")
            return False

# Create default client
client = ResilientDB()

# Example templates
EXAMPLE_TEMPLATES = {
    "create_transaction": """
# Create and send a transaction
from resdb_sdk import ResilientDB, Transaction
import time

# Initialize client
client = ResilientDB('https://crow.resilientdb.com')

# Create unique transaction ID
tx_id = f"test_{int(time.time())}"

# Create transaction object
transaction = Transaction(id=tx_id, value="Hello from ResilientDB!")

# Send transaction
result = await client.transactions.create(transaction)
print(f"Transaction result: {result}")
""",

    "get_transaction": """
# Retrieve a transaction
from resdb_sdk import ResilientDB

# Initialize client
client = ResilientDB('https://crow.resilientdb.com')

# Transaction ID to retrieve
tx_id = "test_1234567890"

# Get transaction
result = await client.transactions.retrieve(tx_id)
print(f"Retrieved transaction: {result}")
""",

    "health_check": """
# Check ResilientDB endpoint health
from resdb_sdk import ResilientDB

client = ResilientDB('https://crow.resilientdb.com')
is_healthy = await client.is_healthy()
print(f"Endpoint is {'healthy' if is_healthy else 'unhealthy'}")
"""
}
