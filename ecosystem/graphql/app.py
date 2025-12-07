#
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
#
#

import tempfile
import os
import sys
import subprocess
import re
import json
import strawberry
import typing
import ast
from pathlib import Path
from typing import Optional, List, Any
from flask import Flask
from flask_cors import CORS
from strawberry.flask.views import GraphQLView

# --- Local Imports ---
from resdb_driver import Resdb
from resdb_driver.crypto import generate_keypair
from json_scalar import JSONScalar 

# --- Vector Indexing Imports ---
from sentence_transformers import SentenceTransformer

# --- Configuration ---
db_root_url = "localhost:18000"
protocol = "http://"
fetch_all_endpoint = "/v1/transactions"
db = Resdb(db_root_url)

# --- Vector Indexing Scripts Path Configuration ---
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
VECTOR_SCRIPT_DIR = os.path.abspath(os.path.join(CURRENT_DIR, "../sdk/vector-indexing"))
PYTHON_EXE = sys.executable

# Add vector script dir to sys.path to allow imports
sys.path.append(VECTOR_SCRIPT_DIR)

# Try importing the manager classes
try:
    from vector_add import VectorIndexManager
    from vector_get import VectorSearchManager
    from vector_delete import VectorDeleteManager
except ImportError as e:
    print(f"Warning: Could not import vector modules. Error: {e}")
    VectorIndexManager = None
    VectorSearchManager = None
    VectorDeleteManager = None

# --- Initialize AI Model & Managers (Run Once) ---
print("Initializing Vector Managers...")
vector_index_manager = None
vector_search_manager = None
vector_delete_manager = None

try:
    # Load model into memory once at startup to avoid per-request overhead
    GLOBAL_MODEL = SentenceTransformer('all-MiniLM-L6-v2')
    
    script_path = Path(VECTOR_SCRIPT_DIR)
    
    if VectorIndexManager:
        vector_index_manager = VectorIndexManager(script_path, GLOBAL_MODEL)
    
    if VectorSearchManager:
        vector_search_manager = VectorSearchManager(script_path, GLOBAL_MODEL)
        
    if VectorDeleteManager:
        vector_delete_manager = VectorDeleteManager(script_path, GLOBAL_MODEL)
        
    print("Vector Managers initialized successfully.")
except Exception as e:
    print(f"Error initializing vector managers: {e}")


app = Flask(__name__)
CORS(app) # This will enable CORS for all routes

# --- GraphQL Types ---

@strawberry.type
class RetrieveTransaction:
    id: str
    version: str
    amount: int
    uri: str
    type: str
    publicKey: str
    operation: str
    metadata: typing.Optional["str"]
    asset: JSONScalar
    signerPublicKey: str

@strawberry.type
class CommitTransaction:
    id: str

@strawberry.input
class PrepareAsset:
    operation: str
    amount: int
    signerPublicKey: str
    signerPrivateKey: str
    recipientPublicKey: str
    asset: JSONScalar

# New Type for Vector Search Results
@strawberry.type
class VectorSearchResult:
    text: str
    score: float

# --- Query ---

@strawberry.type
class Query:
    @strawberry.field
    def getTransaction(self, id: strawberry.ID) -> RetrieveTransaction:
        data = db.transactions.retrieve(txid=id)
        payload = RetrieveTransaction(
            id=data["id"],
            version=data["version"],
            amount=data["outputs"][0]["amount"],
            uri=data["outputs"][0]["condition"]["uri"],
            type=data["outputs"][0]["condition"]["details"]["type"],
            publicKey=data["outputs"][0]["condition"]["details"]["public_key"],
            signerPublicKey=data["inputs"][0]["owners_before"][0],
            operation=data["operation"],
            metadata=data["metadata"],
            asset=data["asset"]
        )
        return payload
    
    @strawberry.field
    def count_cats(self) -> str:
        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode="w+", delete=False) as tmp_file:
            tmp_path = tmp_file.name

            #Write to file
            lines = ["cat", "cat", "cat", "mouse", "cat"]
            for line in lines:
                tmp_file.write(line + "\n")

        # Count number of cats
        cat_count = 0
        with open(tmp_path, "r") as f:
            for line in f:
                if "cat" in line.strip():
                    cat_count += 1

        #Delete temporary file
        os.remove(tmp_path)

        #return number of cats
        return f'The word "cat" appears {cat_count} times'
    
    @strawberry.field
    def getAllVectors(self) -> List[VectorSearchResult]:
        """Search for all texts"""
        results = []
        raw_values = vector_search_manager.get_all_values()
        for val in raw_values:
            # For 'show all', we typically don't have a similarity score, or it's N/A
            results.append(VectorSearchResult(text=val, score=1.0))
        return results

    # --- New: Vector Search Query (Optimized) ---
    @strawberry.field
    def searchVector(self, text: str = None, k: int = 1) -> List[VectorSearchResult]:
        """Search for similar texts using the in-memory manager."""
        results = []
        
        if not vector_search_manager:
            print("Error: Vector search manager not initialized.")
            return []

        if text is None:
            # Show all functionality
            raw_values = vector_search_manager.get_all_values()
            for val in raw_values:
                # For 'show all', we typically don't have a similarity score, or it's N/A
                results.append(VectorSearchResult(text=val, score=1.0))
        else:
            # Search functionality
            search_results = vector_search_manager.search(text, k)
            for item in search_results:
                results.append(VectorSearchResult(
                    text=item['text'],
                    score=item['score']
                ))
        
        return results

# --- Mutation ---

@strawberry.type
class Mutation:
    @strawberry.mutation
    def postTransaction(self, data: PrepareAsset) -> CommitTransaction:
        prepared_token_tx = db.transactions.prepare(
        operation=data.operation,
        signers=data.signerPublicKey,
        recipients=[([data.recipientPublicKey], data.amount)],
        asset=data.asset,
        )

        # fulfill the tnx
        fulfilled_token_tx = db.transactions.fulfill(prepared_token_tx, private_keys=data.signerPrivateKey)

        id = db.transactions.send_commit(fulfilled_token_tx)[4:] # Extract ID
        payload = CommitTransaction(
            id=id
        )
        return payload

    # --- New: Vector Add Mutation (Optimized) ---
    @strawberry.mutation
    def addVector(self, text: str) -> str:
        """Add a text to the vector index using the in-memory manager."""
        if vector_index_manager:
            return vector_index_manager.add_value(text)
        else:
            return "Error: Vector index manager not initialized."

    # --- New: Vector Delete Mutation (Optimized) ---
    @strawberry.mutation
    def deleteVector(self, text: str) -> str:
        """Delete a text from the vector index using the in-memory manager."""
        if vector_delete_manager:
            return vector_delete_manager.delete_value(text)
        else:
            return "Error: Vector delete manager not initialized."


schema = strawberry.Schema(query=Query, mutation=Mutation)

app.add_url_rule(
    "/graphql",
    view_func=GraphQLView.as_view("graphql_view", schema=schema),
)

if __name__ == "__main__":
    app.run(port="8000")