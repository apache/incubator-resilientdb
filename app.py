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

from resdb_driver import Resdb
from resdb_driver.crypto import generate_keypair

db_root_url = "localhost:18000"
protocol = "http://"
fetch_all_endpoint = "/v1/transactions"
db = Resdb(db_root_url)

import strawberry
import typing
import ast
from typing import Optional, List

from flask import Flask
from flask_cors import CORS

app = Flask(__name__)
CORS(app) # This will enable CORS for all routes

from strawberry.flask.views import GraphQLView

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
    asset: strawberry.JSON
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
    asset: strawberry.JSON

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

schema = strawberry.Schema(query=Query, mutation=Mutation)

app.add_url_rule(
    "/graphql",
    view_func=GraphQLView.as_view("graphql_view", schema=schema),
)

if __name__ == "__main__":
    app.run(port="8000")
