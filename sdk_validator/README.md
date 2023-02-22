# resdb validator code in python

## Requirements
- python3.9+
- get the python version from `$ python3 --version`
- `$ python3 -m venv venv`
- `$ source venv/bin/activate`
- `$ pip install -r requirements.txt`
## entrypoint
`validator.py`
    call the `is_valid_tx(tx_dict)` function with the transaction json (tx_dict) as the argument.

## Transaction Validation
A transaction is said to be valid if it satisfies certain condtions or rules.

We employ a simpler version of the transaction spec and validation rules specified by [BigchainDB](https://github.com/bigchaindb/BEPs/tree/master/13#transaction-validation)

 #### JSON Schema Validation
 The json structure of a transaction should be the transaction spec v2 of BigchainDB 
 #### The output.amount Rule
    For all output.amount must be an integer between 1 and 9×10^18, inclusive. The reason for the upper bound is to keep amount within what a server can comfortably represent using a 64-bit signed integer, i.e. 9×10^18 is less than 2^63.
 #### The Duplicate Transaction Rule
    If a transaction is a duplicate of a previous transaction, then it’s invalid. A quick way to check that is by checking to see if a transaction with the same transaction ID is already stored.
    A transaction ID is the hash of the transaction.

 ####  The TRANSFER Transaction Rules
    if a transaction is a `TRANSFER` transaction:

    - TODO: If an input attempts to fulfill an output that has already been fulfilled (i.e. spent or transferred) by a previous valid transaction, then the transaction is invalid. (You don’t have to check if the fulfillment string is valid.)
    - If two or more inputs (in the transaction being validated) attempt to fulfill the same output, then the transaction is invalid. (You don’t have to check any fulfillment strings.)
    -  The sum of the amounts on the inputs must equal the sum of the amounts on the outputs. In other words, a TRANSFER transaction can’t create or destroy asset shares.
    
    For all inputs, if input.fulfills points to:
    - a transaction that doesn’t exist, then it’s invalid.
    - a transaction that’s invalid, then it’s invalid. (This check may be skipped if invalid transactions are never kept.)
    - a transaction output that doesn’t exist, then it’s invalid.
    - a transaction with an asset ID that’s different from this transaction’s asset ID, then this transaction is invalid. (The asset ID of a CREATE transaction is the same as the transaction ID. The asset ID of a TRANSFER transaction is asset.id.)
Note: The first two rules prevent double spending.

#### The input.fulfillment Rule
Regardless of whether the transaction is a CREATE or TRANSFER transaction: For all inputs, input.fulfillment must be valid.
