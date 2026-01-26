import asyncio
import json
from graphql_client import GraphQLClient

async def test_tools():
    client = GraphQLClient()
    
    # Test set operation
    print("Testing set operation...")
    result = await client.set_key_value("test-key", "test-value")
    print(f"Set result: {json.dumps(result, indent=2)}")
    
    # Test get operation
    print("\nTesting get operation...")
    result = await client.get_key_value("test-key")
    print(f"Get result: {json.dumps(result, indent=2)}")
    
    # Test getTransaction (requires valid transaction ID)
    # print("\nTesting getTransaction...")
    # result = await client.get_transaction("YOUR_TRANSACTION_ID")
    # print(f"GetTransaction result: {json.dumps(result, indent=2)}")

if __name__ == "__main__":
    asyncio.run(test_tools())
