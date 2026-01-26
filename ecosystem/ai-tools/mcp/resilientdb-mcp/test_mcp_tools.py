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
