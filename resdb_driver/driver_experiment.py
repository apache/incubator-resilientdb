#%%[markdown]
## Overview
"""
3 main steps:
    1. prepare the payload
    2. Fulfilling the prepared transaction payload
    3. send the set command over the c++ tcp endpoint

- Transaction must be signed before being sent, the fulfillment must be provided by the client.
- Cryptographic signatures, the payloads need to be fully prepared and signed on the client side. This prevents the server(s) from tampering with the provided data.
## Transaction component
```
{
    "id": id,
    "version": version,
    "inputs": inputs,
    "outputs": outputs,
    "operation": operation,
    "asset": asset,
    "metadata": metadata
}```

Transaction inputs and outputs are the mechanism by which control or ownership of an asset is transferred

### Input overview
an input is a pointer to an output of a previous transaction. It specifies to whom an asset belonged before and it provides a proof that the conditions required to transfer the ownership of that asset (e.g. a person needs to sign) are fulfilled. In a CREATE transaction, there is no previous owner, so an input in a CREATE transaction simply specifies who the person is that is registering the object (this is usually the same as the initial owner of the asset). In a TRANSFER transaction, an input contains a proof that the user is authorized to “spend” (transfer or update) this particular output. In practical terms, this means that with the input, a user is stating which asset (e.g. the bike) should be transferred. He also demonstrates that he or she is authorized to do the transfer of that asset

### Output overview
# A transaction output specifies the conditions that need to be fulfilled to change the ownership of a specific asset. For instance: to transfer a NFT, a person needs to sign the transaction with his or her private key. This also implicitly contains the information that the public key associated with that private key is the current owner of the asset.



"""
#%%[markdown]
## creating user
from email import message
from cryptoconditions import Ed25519Sha256
import base58
from resdb_driver.crypto import generate_keypair

alice = generate_keypair()


#%%[markdown]
## Prepare transaction
#%%
### operation
operation = "CREATE"

### asset
asset = {
    "data": {
        "NFT": {
            "size": "10x10",
            "hash": "abcd1234",
        },
    },
}

### metadata
metadata = {"price": "$10"}

#%%
### outputs
"""
The purpose of the output condition is to lock the transaction, such that a valid input fulfillment is required to unlock it. In the case of signature-based schemes, the lock is basically a public key, such that in order to unlock the transaction one needs to have the private key.

"""

ed25519 = Ed25519Sha256(public_key=base58.b58decode(alice.public_key))

#%%
ed25519.condition_uri

"""
  +-----------+--------------------------------------+----------------+
   | Field     | Value                                | Description    |
   +-----------+--------------------------------------+----------------+
   | scheme    | "ni:///"                             | The named      |
   |           |                                      | information    |
   |           |                                      | scheme.        |
   |           |                                      |                |
   | hash      | "sha-256"                            | The            |
   | function  |                                      | fingerprint is |
   | name      |                                      | hashed with    |
   |           |                                      | the SHA-256    |
   |           |                                      | digest         |
   |           |                                      | function       |
   |           |                                      |                |
   | fingerpri | "f4OxZX_x_FO5LcGBSKHWXfwtSx-         | The            |
   | nt        | j1ncoSt3SABJtkGk"                    | fingerprint    |
   |           |                                      | for this       |
   |           |                                      | condition.     |
   |           |                                      |                |
   | type      | "preimage-sha-256"                   | This is a      |
   |           |                                      | PREIMAGE-      |
   |           |                                      | SHA-256        |
   |           |                                      | (Section 8.1)  |
   |           |                                      | condition.     |
   |           |                                      |                |
   | cost      | "12"                                 | The            |
   |           |                                      | fulfillment    |
   |           |                                      | payload is 12  |
   |           |                                      | bytes long,    |
   |           |                                      | therefore the  |
   |           |                                      | cost is 12.    |
   +-----------+--------------------------------------+----------------+

   https://datatracker.ietf.org/doc/html/draft-thomas-crypto-conditions-02#section-8.1

"""
# So now we have a condition URI for Alice’s public key.

#%%
condition_details = {
    "type": ed25519.TYPE_NAME,
    "public_key": base58.b58encode(ed25519.public_key).decode(),
}

output = {
    "amount": "1",
    "condition": {
        "details": condition_details,
        "uri": ed25519.condition_uri,
    },
    "public_keys": (alice.public_key,),  # TODO make it a single value
}
outputs = (output,)

## Each output indicates the crypto-conditions which must be satisfied by anyone wishing to spend/transfer that output.


#%%
# input

input_ = {"fulfillment": None, "fulfills": None, "owners_before": (alice.public_key,)}

"""
The fulfills field is empty because it’s a CREATE operation;
The 'fulfillment' value is None as it will be set during the fulfillment step; and
The 'owners_before' field identifies the issuer(s) of the asset that is being created.
"""

inputs = (input_,)

#%%

creation_tx = {
    "asset": asset,
    "metadata": metadata,
    "operation": operation,
    "outputs": outputs,
    "inputs": inputs,
    "id": None,
}

creation_tx

## Id will be set during the fulfillment step

#%%[markdown]
## fulfilled transaction

#%%
from sha3 import sha3_256
import json
from cryptoconditions.crypto import Ed25519SigningKey

ed25519.to_dict()
message = json.dumps(
    creation_tx, sort_keys=True, separators=(",", ":"), ensure_ascii=False
)

message = sha3_256(message.encode())
ed25519.sign(message.digest(), base58.b58decode(alice.private_key))
fulfillment_uri = ed25519.serialize_uri()
creation_tx["inputs"][0]["fulfillment"] = fulfillment_uri

#%%
# ID - SHA3-256 hash of the entire transaction
json_str_tx = json.dumps(
    creation_tx,
    sort_keys=True,
    separators=(",", ":"),
    ensure_ascii=False,
)
creation_txid = sha3_256(json_str_tx.encode()).hexdigest()
creation_tx["id"] = creation_txid


#%%[markdown]
## Transfer

#%%
import json

import base58
import sha3
from cryptoconditions import Ed25519Sha256

from resdb_driver.crypto import generate_keypair

bob = generate_keypair()

operation = "TRANSFER"
asset = {"id": creation_tx["id"]}
metadata = None

ed25519 = Ed25519Sha256(public_key=base58.b58decode(bob.public_key))

output = {
    "amount": "1",
    "condition": {
        "details": {
            "type": ed25519.TYPE_NAME,
            "public_key": base58.b58encode(ed25519.public_key).decode(),
        },
        "uri": ed25519.condition_uri,
    },
    "public_keys": (bob.public_key,),
}
outputs = (output,)

input_ = {
    "fulfillment": None,
    "fulfills": {
        "transaction_id": creation_tx["id"],
        "output_index": 0,
    },
    "owners_before": (alice.public_key,),
}
inputs = (input_,)

transfer_tx = {
    "asset": asset,
    "metadata": metadata,
    "operation": operation,
    "outputs": outputs,
    "inputs": inputs,
    "id": None,
}

message = json.dumps(
    transfer_tx,
    sort_keys=True,
    separators=(",", ":"),
    ensure_ascii=False,
)

message = sha3.sha3_256(message.encode())

message.update(
    "{}{}".format(
        transfer_tx["inputs"][0]["fulfills"]["transaction_id"],
        transfer_tx["inputs"][0]["fulfills"]["output_index"],
    ).encode()
)

ed25519.sign(message.digest(), base58.b58decode(alice.private_key))

fulfillment_uri = ed25519.serialize_uri()

transfer_tx["inputs"][0]["fulfillment"] = fulfillment_uri

json_str_tx = json.dumps(
    transfer_tx,
    sort_keys=True,
    separators=(",", ":"),
    ensure_ascii=False,
)

transfer_txid = sha3.sha3_256(json_str_tx.encode()).hexdigest()

transfer_tx["id"] = transfer_txid
