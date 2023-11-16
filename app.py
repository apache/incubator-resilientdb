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
from filter import filter_by_keys

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
    asset: str

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
    asset: str

@strawberry.input
class UpdateAsset:
    id: str
    operation: typing.Optional["str"]
    amount: typing.Optional["int"]
    signerPublicKey: str
    signerPrivateKey: str
    recipientPublicKey: typing.Optional["str"]
    asset: typing.Optional["str"]

@strawberry.input
class FilterKeys:
    ownerPublicKey: Optional[str]
    recipientPublicKey: Optional[str]

@strawberry.type
class Keys:
    publicKey: str
    privateKey: str

def update(data):
    record = db.transactions.retrieve(data.id)
    prepared_token_tx = db.transactions.prepare(
    operation=record["operation"] if data.operation == "" else data.operation,
    signers=data.signerPublicKey,
    recipients=[([record["outputs"][0]["condition"]["details"]["public_key"] if data.recipientPublicKey == "" else data.recipientPublicKey], record["outputs"][0]["amount"] if data.amount == "" else data.amount)],
    asset=record["asset"] if data.asset == "" else ast.literal_eval(data.asset),
    )

    # fulfill the tnx
    fulfilled_token_tx = db.transactions.fulfill(prepared_token_tx, private_keys=data.signerPrivateKey)

    id = db.transactions.send_commit(fulfilled_token_tx)[4:] # Extract ID
    data = db.transactions.retrieve(txid=id)
    payload = RetrieveTransaction(
        id=data["id"],
        version=data["version"],
        amount=data["outputs"][0]["amount"],
        uri=data["outputs"][0]["condition"]["uri"],
        type=data["outputs"][0]["condition"]["details"]["type"],
        publicKey=data["outputs"][0]["condition"]["details"]["public_key"],
        operation=data["operation"],
        metadata=data["metadata"],
        asset=str(data["asset"])
    )
    return payload

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
            operation=data["operation"],
            metadata=data["metadata"],
            asset=str(data["asset"])
        )
        return payload
    
    @strawberry.field
    def getFilteredTransactions(self, filter: Optional[FilterKeys]) -> List[RetrieveTransaction]:
        url = f"{protocol}{db_root_url}{fetch_all_endpoint}"
        if filter.ownerPublicKey != None:
            filter.ownerPublicKey = filter.ownerPublicKey if filter.ownerPublicKey.strip() else None
        if filter.recipientPublicKey != None:
            filter.recipientPublicKey = filter.recipientPublicKey if filter.recipientPublicKey.strip() else None
        json_data = filter_by_keys(url, filter.ownerPublicKey, filter.recipientPublicKey)
        records = []
        for data in json_data:
            try:
                records.append(RetrieveTransaction(
                id=data["id"],
                version=data["version"],
                amount=data["outputs"][0]["amount"],
                uri=data["outputs"][0]["condition"]["uri"],
                type=data["outputs"][0]["condition"]["details"]["type"],
                publicKey=data["outputs"][0]["condition"]["details"]["public_key"],
                operation=data["operation"],
                metadata=data["metadata"],
                asset=str(data["asset"])
                ))
            except Exception as e:
                print(e)
            return records

@strawberry.type
class Mutation:
    @strawberry.mutation
    def postTransaction(self, data: PrepareAsset) -> CommitTransaction:
        prepared_token_tx = db.transactions.prepare(
        operation=data.operation,
        signers=data.signerPublicKey,
        recipients=[([data.recipientPublicKey], data.amount)],
        asset=ast.literal_eval(data.asset),
        )

        # fulfill the tnx
        fulfilled_token_tx = db.transactions.fulfill(prepared_token_tx, private_keys=data.signerPrivateKey)

        id = db.transactions.send_commit(fulfilled_token_tx)[4:] # Extract ID
        payload = CommitTransaction(
            id=id
        )
        return payload
    
    @strawberry.mutation
    def updateTransaction(self, data: UpdateAsset) -> RetrieveTransaction:
        return update(data)
    
    @strawberry.mutation
    def updateMultipleTransaction(self, data: List[UpdateAsset]) -> List[RetrieveTransaction]:
        result = []
        for transaction in data:
            result.append(update(transaction))
        return result

    @strawberry.mutation
    def generateKeys(self) -> Keys:
        keys = generate_keypair()
        payload = Keys(
            publicKey=keys.public_key,
            privateKey=keys.private_key
        )
        return payload


schema = strawberry.Schema(query=Query, mutation=Mutation)

app.add_url_rule(
    "/graphql",
    view_func=GraphQLView.as_view("graphql_view", schema=schema),
)

if __name__ == "__main__":
    app.run(port="8000")
