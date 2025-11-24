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

"""
ResilientDB Python SDK for Web Playground
This is a simplified version of the official ResilientDB Python SDK, adapted for web playground use.
"""
import json
import time
from typing import Dict, Any, Optional, List, Union
from pyodide.http import pyfetch

RESDB_REST_URL = "https://crow.resilientdb.com"

class TransactionMetadata:
    def __init__(self, **kwargs):
        self.data = kwargs
    
    def to_dict(self) -> Dict[str, Any]:
        return self.data

class Transaction:
    def __init__(self, id: str, value: str, metadata: Optional[Union[Dict, TransactionMetadata]] = None):
        self.id = str(id)
        self.value = str(value)
        self.type = "kv"
        self.metadata = metadata if isinstance(metadata, TransactionMetadata) else TransactionMetadata(**(metadata or {}))
    
    def to_dict(self) -> Dict[str, Any]:
        data = {
            "id": self.id,
            "value": self.value,
            "type": self.type
        }
        if self.metadata and self.metadata.data:
            data["metadata"] = self.metadata.to_dict()
        return data

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
    
    async def get_info(self) -> Dict[str, Any]:
        """Get ResilientDB node information"""
        try:
            response = await pyfetch(
                f"{self.url}/v1/info",
                method="GET",
                headers={"Accept": "application/json"}
            )
            
            if response.status == 200:
                return await response.json()
            return {"error": "Failed to get node info"}
            
        except Exception as e:
            return {"error": f"Failed to get node info: {str(e)}"}

# Create default client
client = ResilientDB()

# Example templates
EXAMPLE_TEMPLATES = {
    "create_transaction": '''"""
Example: Create and send a simple transaction (without metadata)
"""
from resdb_sdk import ResilientDB, Transaction
import json
import time

# Initialize ResilientDB client
client = ResilientDB('https://crow.resilientdb.com')

# Create unique ID using timestamp
unique_id = f"test_{int(time.time())}"

# Create transaction (without metadata)
transaction = Transaction(
    id=unique_id,
    value="Hello from ResilientDB!"
)

print(f"Creating transaction with ID: {unique_id}")
print("Transaction data:")
print(json.dumps(transaction.to_dict(), indent=2))

print("\\nSending transaction...")
result = await client.transactions.create(transaction)
print("Response:")
print(json.dumps(result, indent=2))''',

    "create_transaction_with_metadata": """
# Create and send a transaction with metadata
from resdb_sdk import ResilientDB, Transaction, TransactionMetadata
import time

# Initialize client
client = ResilientDB('https://crow.resilientdb.com')

# Create unique transaction ID
tx_id = f"test_{int(time.time())}"

# Create metadata
metadata = TransactionMetadata(
    timestamp=time.time(),
    source="playground",
    tags=["test", "example"]
)

# Create transaction object with metadata
transaction = Transaction(
    id=tx_id,
    value="Hello from ResilientDB!",
    metadata=metadata
)

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

    "complete_workflow": """
# Complete workflow example
from resdb_sdk import ResilientDB, Transaction, TransactionMetadata
import json
import time

# Initialize client
client = ResilientDB('https://crow.resilientdb.com')

# Step 1: Create and send transaction
print("Step 1: Creating and sending transaction...")

# Generate unique ID using timestamp
unique_id = f"test_{int(time.time())}"

# Create metadata
metadata = TransactionMetadata(
    timestamp=time.time(),
    source="playground",
    workflow="complete_example"
)

# Create transaction
transaction = Transaction(
    id=unique_id,
    value="Complete workflow test",
    metadata=metadata
)

print(f"Transaction ID: {unique_id}")
print("Transaction data:")
print(json.dumps(transaction.to_dict(), indent=2))

# Send transaction
send_result = await client.transactions.create(transaction)
print("\\nPOST Response:")
print(json.dumps(send_result, indent=2))

# Step 2: Retrieve the transaction
print("\\nStep 2: Retrieving the transaction...")
get_result = await client.transactions.retrieve(unique_id)
print("GET Response:")
print(json.dumps(get_result, indent=2))

print("\\nWorkflow complete!")
"""
}
