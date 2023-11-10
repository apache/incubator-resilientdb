#%%

from resdb_driver import Resdb

db_root_url = "http://127.0.0.1:18000"

db = Resdb(db_root_url)
from resdb_driver.crypto import generate_keypair

alice, bob = generate_keypair(), generate_keypair()
#%%

#%%

"""
----- SET OPERATION -----
Creating a transaction with a digital asset and issuing 10 tokens for it

The owner's private key is used for signing the transaction and the recipient is identified by their public key
In the example below: Alice is the owner and Bob is the recipient
"""

"""
Creating a digital asset, you can add any data here in the form of a Python dictionary
Example: {"data": {"key": "value"}}
"""
asset = {
    "data": {
        "description": "Any data goes here."
    },
}

# The number of tokens you want to issue
amount = 10

# Preparing the transaction
prepared_token_tx = db.transactions.prepare(
    operation="CREATE",
    signers=alice.public_key,
    recipients=[([bob.public_key], amount)],
    asset=asset,
)

#%%
# fulfill the txn by signing it with the private key
fulfilled_token_tx = db.transactions.fulfill(
    prepared_token_tx, private_keys=alice.private_key
)

#%%
# send the txn to ResDB, and completion of SET operation
db.transactions.send_commit(fulfilled_token_tx)

#%%
transfer_asset = {"id": fulfilled_token_tx["id"]}
output_index = 0
output = fulfilled_token_tx["outputs"][output_index]
transfer_input = {
    "fulfillment": output["condition"]["details"],
    "fulfills": {"output_index": output_index, "transaction_id": transfer_asset["id"]},
    "owners_before": output["public_keys"],
}

#%%
prepared_transfer_tx = db.transactions.prepare(
    operation="TRANSFER",
    asset=transfer_asset,
    inputs=transfer_input,
    recipients=[([alice.public_key], 3), ([bob.public_key], 8)],
)

#%%
# fulfill the transfer txn by signing it with the private key
fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx, private_keys=bob.private_key
)

#%%
# send the transfer txn to ResDB
sent_transfer_tx = db.transactions.send_commit(fulfilled_transfer_tx)


# %%

"""
----- GET OPERATION -----
Retrieving a transaction using the transaction ID

fulfilled_token_tx["id"] contains the ID of the transaction that was signed earlier, completion of the SET transaction via db.transactions.send_commit(fulfilled_token_tx) also returns this ID
"""
## set `testId` with the txid for the transaction to retrieve
retrieve_id = fulfilled_token_tx["id"]
retrieved_txn = db.transactions.retrieve(txid=retrieve_id)

# You can now print the transaction information using the returned value
# print(retrieved_txn)

#%%
# valide a tx object
from resdb_driver.validate import Transaction

t = Transaction.from_dict(fulfilled_token_tx)

try:
    result = t.validate(resdb=db)
    print("The retrieved txn is successfully validated")
except:
    print("An exception occurred")