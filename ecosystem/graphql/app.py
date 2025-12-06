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
from pathlib import Path

from resdb_driver import Resdb
from resdb_driver.crypto import generate_keypair

# --- Configuration ---
db_root_url = "localhost:18000"
protocol = "http://"
fetch_all_endpoint = "/v1/transactions"
db = Resdb(db_root_url)

# --- Vector Indexing Scripts Path Configuration ---
# app.py is in ecosystem/graphql/
# scripts are in ecosystem/sdk/vector-indexing/
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
VECTOR_SCRIPT_DIR = os.path.abspath(os.path.join(CURRENT_DIR, "../sdk/vector-indexing"))
PYTHON_EXE = sys.executable

def run_vector_script(script_name: str, args: list) -> tuple[bool, str]:
    """Helper to run python scripts located in the vector-indexing directory."""
    script_path = os.path.join(VECTOR_SCRIPT_DIR, script_name)
    
    if not os.path.exists(script_path):
        return False, f"Script not found: {script_path}"

    command = [PYTHON_EXE, script_path] + args
    try:
        # Run script with the working directory set to where the script is
        # (because the scripts rely on relative paths like ./saved_data)
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            cwd=VECTOR_SCRIPT_DIR 
        )
        if result.returncode != 0:
            return False, result.stderr + "\n" + result.stdout
        return True, result.stdout.strip()
    except Exception as e:
        return False, str(e)

import strawberry
import typing
import ast
import json

from typing import Optional, List, Any
from flask import Flask
from flask_cors import CORS

from json_scalar import JSONScalar 

app = Flask(__name__)
CORS(app) # This will enable CORS for all routes

from strawberry.flask.views import GraphQLView

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

    # --- New: Vector Search Query ---
    @strawberry.field
    def searchVector(self, text: str = None, k: int = 1) -> List[VectorSearchResult]:
        """Search for similar texts using the HNSW index."""
        success = False
        output = ""
        if text is None:
            success, output = run_vector_script("vector_get.py", ["--show_all"])
        else:
            success, output = run_vector_script("vector_get.py", ["--value", text, "--k_matches", str(k)])

        results = []
        if not success:
            # Log error internally if needed, returning empty list or raising error
            print(f"Vector search failed: {output}")
            return []

        # Parse the output from vector_get.py (e.g. "1. hello // (similarity score: 0.123)")
        for line in output.splitlines():
            match = re.search(r'^\d+\.\s+(.*?)\s+//\s+\(similarity score:\s+([0-9.]+)\)', line)
            if match:
                results.append(VectorSearchResult(
                    text=match.group(1),
                    score=float(match.group(2))
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

    # --- New: Vector Add Mutation ---
    @strawberry.mutation
    def addVector(self, text: str) -> str:
        """Add a text to the vector index."""
        success, output = run_vector_script("vector_add.py", ["--value", text])
        if success:
            return "Success: Added to index."
        elif "already saved" in output:
            return "Skipped: Value already exists."
        else:
            return f"Error: {output}"

    # --- New: Vector Delete Mutation ---
    @strawberry.mutation
    def deleteVector(self, text: str) -> str:
        """Delete a text from the vector index."""
        success, output = run_vector_script("vector_delete.py", ["--value", text])
        if success:
            return "Success: Deleted from index."
        else:
            return f"Error: {output}"


schema = strawberry.Schema(query=Query, mutation=Mutation)

app.add_url_rule(
    "/graphql",
    view_func=GraphQLView.as_view("graphql_view", schema=schema),
)

if __name__ == "__main__":
    app.run(port="8000")