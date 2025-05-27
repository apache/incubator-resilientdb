export const PYTHON_SDK_EXAMPLE_TEMPLATES = [
    {
      value: 'default',
      label: 'Welcome - Start Here',
      code: `"""
  Welcome to the ResilientDB Python Playground!
  
  This interactive environment allows you to test and experiment with ResilientDB's key-value store 
  using our Python SDK. The code here matches the patterns used in the official ResilientDB Python SDK.
  
  Available Features:
  1. Basic Transaction Operations
     - Create simple transactions
     - Create transactions with metadata
     - Retrieve transactions by ID
  
  Choose an example from the dropdown menu above to get started!
  
  Note: Each transaction needs a unique ID. Our examples use timestamps to ensure uniqueness.
  """`,
    },
    {
      value: 'send_transaction',
      label: 'Send Simple Transaction',
      code: `"""
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
  print(json.dumps(result, indent=2))`,
    },
    {
      value: 'send_transaction_with_metadata',
      label: 'Send Transaction with Metadata',
      code: `"""
  Example: Create and send a transaction with metadata
  """
  from resdb_sdk import ResilientDB, Transaction, TransactionMetadata
  import json
  import time
  
  # Initialize ResilientDB client
  client = ResilientDB('https://crow.resilientdb.com')
  
  # Create unique ID using timestamp
  unique_id = f"test_{int(time.time())}"
  
  # Create metadata
  metadata = TransactionMetadata(
      timestamp=time.time(),
      source="playground",
      tags=["test", "example"]
  )
  
  # Create transaction with metadata
  transaction = Transaction(
      id=unique_id,
      value="Hello from ResilientDB!",
      metadata=metadata
  )
  
  print(f"Creating transaction with ID: {unique_id}")
  print("Transaction data:")
  print(json.dumps(transaction.to_dict(), indent=2))
  
  print("\\nSending transaction...")
  result = await client.transactions.create(transaction)
  print("Response:")
  print(json.dumps(result, indent=2))`,
    },
    {
      value: 'get_transaction',
      label: 'Get Transaction',
      code: `"""
  Example: Retrieve a transaction from ResilientDB
  """
  from resdb_sdk import ResilientDB
  import json
  
  # Initialize ResilientDB client
  client = ResilientDB('https://crow.resilientdb.com')
  
  # Transaction ID to retrieve
  # Replace this with an ID from a previously sent transaction
  tx_id = "test_1234567890"
  
  print(f"Retrieving transaction with ID: {tx_id}")
  print("\\nSending GET request...")
  
  result = await client.transactions.retrieve(tx_id)
  print("\\nResponse:")
  print(json.dumps(result, indent=2))`,
    },
    {
      value: 'complete_workflow',
      label: 'Complete Workflow',
      code: `"""
  Example: Complete workflow demonstrating transaction creation and retrieval
  """
  from resdb_sdk import ResilientDB, Transaction, TransactionMetadata
  import json
  import time
  
  # Initialize ResilientDB client
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
  
  print("\\nWorkflow complete!")`,
    },
  ];


export const RESILIENTDB_GRAPHQL_EXAMPLE_TEMPLATES = [
    {
        value: 'rest_api_examples',
        label: 'Rest API Examples',
        code: `
from pyodide.http import pyfetch
import json

# Initialize connection
BASE_URL = 'https://crow.resilientdb.com/v1'

# Set a transaction
async def set_transaction(key, value):
    response = await pyfetch(
        f'{BASE_URL}/transactions/commit',
        method='POST',
        headers={'Content-Type': 'application/json'},
        body=json.dumps({
            'id': key,
            'value': value
        })
    )
    return await response.json()

# Get a transaction
async def get_transaction(key):
    response = await pyfetch(f'{BASE_URL}/transactions/{key}')
    return await response.json()

# Example usage
async def main():
    # Set a transaction
    result = await set_transaction('test_key', 'test_value')
    print('Set result:', result)
    
    # Get a transaction
    value = await get_transaction('test_key')
    print('Get result:', value)

# Run the example
await main()
`,
    },
    {
        value: 'graphql_query_example',
        label: 'Get Transaction (Query)',
        code: `
from pyodide.http import pyfetch
import json

# GraphQL endpoint
url = "https://cloud.resilientdb.com/graphql"

# Query and variables
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

variables = {
    "id": "6358ee73fb6bf5c659676ccc8476ebb6ea738b47a101661697ef379e5ea0212b"
}

# Prepare the payload
payload = {
    "query": query,
    "variables": variables
}

async def get_transaction():
    # Make the request
    response = await pyfetch(
        url,
        method="POST",
        headers={"Content-Type": "application/json"},
        body=json.dumps(payload)
    )
    
    # Get and print the response
    result = await response.json()
    print(json.dumps(result, indent=2))

# Run the example
await get_transaction()
`,
    },
    {
        value: 'graphql_mutation_example',
        label: 'Create Transaction (Mutation)',
        code: `
from pyodide.http import pyfetch
import json

# GraphQL endpoint
url = "https://cloud.resilientdb.com/graphql"

# Drawing ID for the asset
drawing_id = "drawing_testing_random_seed"

# Mutation and variables
mutation = """
mutation Test($data: PrepareAsset!) {
    postTransaction(data: $data) {
        id
    }
}
"""

variables = {
    "data": {
        "operation": "CREATE",
        "amount": 1,
        "signerPublicKey": "FbUGKzKnSgh6bKRw8sxdzaCq1NMjGT6FVeAWLot5bCa1",
        "signerPrivateKey": "5EzirRSQvWHwtekrg4TYtxBbdJbtgvG25pepGZRWJneC",
        "recipientPublicKey": "FbUGKzKnSgh6bKRw8sxdzaCq1NMjGT6FVeAWLot5bCa1",
        "asset": {
            "data": {
                "drawingId": drawing_id,
                "color": "#000000",
                "lineWidth": 5,
                "pathData": [{"x": 292, "y": 126}],
                "timestamp": 1746736411543,
                "user": "appleseed|1746736304144",
                "brushStyle": "round",
                "order": 1746736411543
            }
        }
    }
}

async def create_transaction():
    # Prepare the payload
    payload = {
        "query": mutation,
        "variables": variables
    }
    
    # Make the request
    response = await pyfetch(
        url,
        method="POST",
        headers={"Content-Type": "application/json"},
        body=json.dumps(payload)
    )
    
    # Get and print the response
    result = await response.json()
    print(json.dumps(result, indent=2))

# Run the example
await create_transaction()
`,
    }
]