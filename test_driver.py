#%%

from resdb_driver import Resdb

db_root_url = "http://127.0.0.1:18000"
#db_root_url = "https://resdb.free.beeceptor.com"

db = Resdb(db_root_url)
from resdb_driver.crypto import generate_keypair

alice, bob = generate_keypair(), generate_keypair()
#%%

# create a digital asset for Alice
game_boy_token = {
    "data": {
        "token_for": {"game_boy": {"serial_number": "LR35902"}},
        "description": "Time share token. Each token equals one hour of usage.",
    },
}

#%%

# prepare the transaction with the digital asset and issue 10 tokens for Bob
prepared_token_tx = db.transactions.prepare(
    operation="CREATE",
    signers=alice.public_key,
    recipients=[([bob.public_key], 10)],
    asset=game_boy_token,
)

#%%
# fulfill the tnx
fulfilled_token_tx = db.transactions.fulfill(
    prepared_token_tx, private_keys=alice.private_key
)

#%%
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

fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx, private_keys=bob.private_key
)

#%%

sent_transfer_tx = db.transactions.send_commit(fulfilled_transfer_tx)


# %%
## replace `testId` with the txid for the transaction to retrieve
db.transactions.retrieve(txid="testId")

#%%
# TODO valide a tx object
# from resdb_driver.validate import Transaction

# t = Transaction.from_dict(fulfilled_token_tx)
# t.validate()
