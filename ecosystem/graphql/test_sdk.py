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

db_root_url = "http://127.0.0.1:18000"

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

# Bob transfers 5 tokens to Alice, and is left with 5 tokens
# The sums of amounts in recipients must match the input amount (10)
prepared_transfer_tx = db.transactions.prepare(
    operation="TRANSFER",
    asset=transfer_asset,
    inputs=transfer_input,
    recipients=[([alice.public_key], 5), ([bob.public_key], 5)],
)

#%%

fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx, private_keys=bob.private_key
)

#%%

sent_transfer_tx = db.transactions.send_commit(fulfilled_transfer_tx)


# %%
## replace `testId` with the txid for the transaction to retrieve
db.transactions.retrieve(txid=sent_transfer_tx)

#%%
# TODO valide a tx object
# from resdb_driver.validate import Transaction

# t = Transaction.from_dict(fulfilled_token_tx)
# t.validate()
