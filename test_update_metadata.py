"""
Example code to update metadata as part of TRANSFER tx

stage 1. Alice creates an asset and sets Bob as the owner with price=100
stage 2. Bob transfers the asset to Alice with price=200
stage 3. Alice transfers the asset to Alice with price=300 (only metadata is changed)
stage 4. Alice transfers the asset to Alice with price=400 (only metadata is changed)

Print the fulfilled_transfer_tx or its equivalent to see the tx items
"""
#%%

from resdb_driver import Resdb
from resdb_driver.transaction import Transaction
from copy import deepcopy

db_root_url = "https://resdb.free.beeceptor.com"

db = Resdb(db_root_url)
from resdb_driver.crypto import generate_keypair

alice, bob = generate_keypair(), generate_keypair()
#%%
# Stage 1
# create a digital asset for Alice
game_boy_token = {
    "data": {
        "token_for": {"game_boy": {"serial_number": "LR35902"}},
        "description": "Time share token. Each token equals one hour of usage.",
    },
}

#%%
# prepare the transaction with the digital asset and issue 1 token for Bob
prepared_token_tx = db.transactions.prepare(
    operation="CREATE",
    signers=alice.public_key,
    recipients=[([bob.public_key], 1)],
    asset=game_boy_token,
)

prepared_token_tx["metadata"] = {"price": "100"} #metadata has to be a dict 
#%%
# fulfill the tnx
fulfilled_token_tx = db.transactions.fulfill(
    prepared_token_tx, private_keys=alice.private_key
)


#%% stage 2
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
    metadata={"price": "200"},
    recipients=[([alice.public_key], 1)],
)

#%%

fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx, private_keys=bob.private_key
)

# stage 3
# Change the metadata 
#%%
transfer_asset = {"id": fulfilled_token_tx["id"]}
output_index = 0
output = fulfilled_transfer_tx["outputs"][output_index]
transfer_input = {
    "fulfillment": output["condition"]["details"],
    "fulfills": {"output_index": output_index, "transaction_id": fulfilled_transfer_tx["id"]},
    "owners_before": output["public_keys"],
}

prepared_transfer_tx = db.transactions.prepare(
    operation="TRANSFER",
    asset=transfer_asset,
    inputs=transfer_input,
    metadata={"price": "300"},
    recipients=[([alice.public_key], 1)],
)
fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx, private_keys=alice.private_key
)

# stage 4
# chage the metadata again

#%%
transfer_asset = {"id": fulfilled_token_tx["id"]}
output_index = 0
output = fulfilled_transfer_tx["outputs"][output_index]
transfer_input = {
    "fulfillment": output["condition"]["details"],
    "fulfills": {"output_index": output_index, "transaction_id": fulfilled_transfer_tx["id"]},
    "owners_before": output["public_keys"],
}

prepared_transfer_tx = db.transactions.prepare(
    operation="TRANSFER",
    asset=transfer_asset,
    inputs=transfer_input,
    metadata={"price": "400"},
    recipients=[([alice.public_key], 1)],
)
fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx, private_keys=alice.private_key
)


#%%
sent_transfer_tx = db.transactions.send_commit(fulfilled_transfer_tx) # use this to commit whenever required

