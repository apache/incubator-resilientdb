from service.sdk_validator.resdb_validator.exceptions import (InvalidSignature,
                                          DuplicateTransaction)
from service.sdk_validator.resdb_validator.transaction import Transaction
from service.sdk_validator.resdb_validator.utils import (validate_txn_obj, validate_key)


class Transaction(Transaction):
    ASSET = 'asset'
    METADATA = 'metadata'
    DATA = 'data'

    def validate(self, resdb=None, current_transactions=[]):
        """Validate transaction spend
        Args:
            resdb (ResDB): an instantiated resdb_validator.ResDB object.
        Returns:
            The transaction (Transaction) if the transaction is valid else it
            raises an exception describing the reason why the transaction is
            invalid.
        Raises:
            ValidationError: If the transaction is invalid
        """
        input_conditions = []

        if self.operation == Transaction.CREATE:
            duplicates = any(txn for txn in current_transactions if txn.id == self.id)
            if resdb and  resdb.is_committed(self.id) or duplicates:
                raise DuplicateTransaction('transaction `{}` already exists'
                                           .format(self.id))

            if not self.inputs_valid(input_conditions):
                raise InvalidSignature('Transaction signature is invalid.')

        elif self.operation == Transaction.TRANSFER:
            self.validate_transfer_inputs(resdb, current_transactions)

        return self

    @classmethod
    def from_dict(cls, tx_body):
        #TODO schema validation
        return super().from_dict(tx_body, True)

    # @classmethod
    # def validate_schema(cls, tx_body):
    #     validate_transaction_schema(tx_body)
    #     validate_txn_obj(cls.ASSET, tx_body[cls.ASSET], cls.DATA, validate_key)
    #     validate_txn_obj(cls.METADATA, tx_body, cls.METADATA, validate_key)
    #     validate_language_key(tx_body[cls.ASSET], cls.DATA)
    #     validate_language_key(tx_body, cls.METADATA)


class FastTransaction:
    """A minimal wrapper around a transaction dictionary. This is useful for
    when validation is not required but a routine expects something that looks
    like a transaction, for example during block creation.

    Note: immutability could also be provided
    """

    def __init__(self, tx_dict):
        self.data = tx_dict

    @property
    def id(self):
        return self.data['id']

    def to_dict(self):
        return self.data
