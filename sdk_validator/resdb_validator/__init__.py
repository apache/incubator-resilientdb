from sdk_validator.resdb_validator.transaction import Transaction 
from sdk_validator.resdb_validator import models                         

Transaction.register_type(Transaction.CREATE, models.Transaction)
Transaction.register_type(Transaction.TRANSFER, models.Transaction)
