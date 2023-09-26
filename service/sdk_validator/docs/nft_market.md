# Building a NFT market with resdb

## How can Sajjad sell his painting to Mo?
1. Sajjad registers his painting as an immutable asset of which he has ownership
2. While registering the asset he would need to share some immutable info such as `name of painting`, `date of creation`, `painter` ect.
3. Optional metadata can be added as well about things like `price`, `currency` ect.
3. Upload the NFT publicly for everyone to see
4. Sell the NFT to Mo

### The above process in terms of transactions
1. Creating the NFT = `CREATE` transaction
2. Uploading the NFT = `SEND` transaction or `send_commit`
3. transferring ownership from Sajjad to Mo = `TRANSFER` transaction

Resdb uses public key cryptography to register and maintain the integrity and security of assets.
Each asset has to be signed with the owner's private key. This provides surity that the owner of the asset is the one making the transactions.

Each transactions generates a unique hash called `transaction ID`.

## Creating and Transferring Assets

#### Importing the important stuff
```python
from resdb_driver import Resdb
from resdb_driver.crypto import generate_keypair
```

#### Generating keys
```python
Sajjad, Mo = generate_keypair(), generate_keypair()
```

#### Set up root url
```python
root_url = 'https://resdb_url:8080'
```

#### PART 1: Creating the Asset and registering it under Sajjad's ownership
```python
db = Nesres(root_url)
painting_asset = {
    'data': {
        'painting': {
            'serialized_img': 'qwertyuioplkjhgfdazxcvbnm' # everything under data is immutable stuff
            'name': 'New Mona Lisa',
            'painter': 'Sajjad'
        },
    },
}

painting_asset_metadata = {
    'date_started': '1/1/2022',
    'date_completed': '12/3/2022',
    'price': '100',
    'currency': 'USD'
}

prepared_creation_tx = db.transactions.prepare(
    operation=’CREATE’,
    signers=Sajjad.public_key,
    asset=painting_asset,
    metadata=painting_asset_metadata
)
 
fulfilled_creation_tx = db.transactions.fulfill(
    prepared_creation_tx,
    private_keys=Sajjad.private_key
)
```

#### Upload the asset to the database
```python
sent_creation_tx = db.transactions.send_commit(fulfilled_creation_tx)
```

#### PART 2: preparing the transfering of asset ownership from Sajjad to Mo
```python
txid = fulfilled_creation_tx['id'] # In reality this transaction id and asset details needs to be fetched from the database
asset_id = txid
transfer_asset = {
    'id': asset_id
}
output_index = 0
output = fulfilled_creation_tx['outputs'][output_index]
transfer_input = {
    'fulfillment': output['condition']['details'],
    'fulfills': {
        ‘output_index’: output_index,
        ‘transaction_id’: fulfilled_creation_tx['id']
    },
    'owners_before': output['public_keys']
}
prepared_transfer_tx = db.transactions.prepare(
    operation=’TRANSFER’,
    asset=transfer_asset,
    inputs=transfer_input,
    recipients=Mo.public_key,
)
fulfilled_transfer_tx = db.transactions.fulfill(
    prepared_transfer_tx,
    private_keys=Sajjad.private_key,
)
```

#### competing the TRANSFER operatiom with a commit
`sent_transfer_tx = resdb.transactions.send_commit(fulfilled_transfer_tx)`

#### Comfirming
```python
print(“Is Mo the owner of the painting?”,
    sent_transfer_tx[‘outputs’][0][‘public_keys’][0] == Mo.public_key)
print(“Was Sajjad the previous owner of the painting?”,
    fulfilled_transfer_tx[‘inputs’][0][‘owners_before’][0] == Sajjad.public_key)
```

#### !Note!
We are using the same transaction model as [bigchaindb transaction model](https://github.com/bigchaindb/BEPs/tree/master/13)


