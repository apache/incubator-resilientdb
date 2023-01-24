class ResdbException(Exception):
    """Base exception for all resdb exceptions."""


class ResdbException(Exception):
    """Base exception for all Resdb exceptions."""


class MissingPrivateKeyError(ResdbException):
    """Raised if a private key is missing."""


class TransportError(ResdbException):
    """Base exception for transport related errors.

    This is mainly for cases where the status code denotes an HTTP error, and
    for cases in which there was a connection error.

    """

    @property
    def status_code(self):
        return self.args[0]

    @property
    def error(self):
        return self.args[1]

    @property
    def info(self):
        return self.args[2]

    @property
    def url(self):
        return self.args[3]


class BadRequest(TransportError):
    """Exception for HTTP 400 errors."""


class NotFoundError(TransportError):
    """Exception for HTTP 404 errors."""


class ServiceUnavailable(TransportError):
    """Exception for HTTP 503 errors."""


class GatewayTimeout(TransportError):
    """Exception for HTTP 503 errors."""


class TimeoutError(ResdbException):
    """Raised if the request algorithm times out."""

    @property
    def connection_errors(self):
        """Returns connection errors occurred before timeout expired."""
        return self.args[0]


HTTP_EXCEPTIONS = {
    400: BadRequest,
    404: NotFoundError,
    503: ServiceUnavailable,
    504: GatewayTimeout,
}


class ResDBError(Exception):
    """Base class for ResDB exceptions."""


class ConfigurationError(ResDBError):
    """Raised when there is a problem with server configuration"""


class DatabaseAlreadyExists(ResDBError):
    """Raised when trying to create the database but the db is already there"""


class DatabaseDoesNotExist(ResDBError):
    """Raised when trying to delete the database but the db is not there"""


class StartupError(ResDBError):
    """Raised when there is an error starting up the system"""


class CyclicBlockchainError(ResDBError):
    """Raised when there is a cycle in the blockchain"""


class KeypairMismatchException(ResDBError):
    """Raised if the private key(s) provided for signing don't match any of the
    current owner(s)
    """


class OperationError(ResDBError):
    """Raised when an operation cannot go through"""


##############################################################################
# Validation errors
#
# All validation errors (which are handleable errors, not faults) should
# subclass ValidationError. However, where possible they should also have their
# own distinct type to differentiate them from other validation errors,
# especially for the purposes of testing.


class ValidationError(ResDBError):
    """Raised if there was an error in validation"""


class DoubleSpend(ValidationError):
    """Raised if a double spend is found"""


class DoubleSpend(ValidationError):
    """Raised if a double spend is found"""


class InvalidHash(ValidationError):
    """Raised if there was an error checking the hash for a particular
    operation
    """


class InputDoesNotExist(ValidationError):
    """Raised if a transaction input does not exist"""


class SchemaValidationError(ValidationError):
    """Raised if there was any error validating an object's schema"""


class InvalidSignature(ValidationError):
    """Raised if there was an error checking the signature for a particular
    operation
    """


class TransactionNotInValidBlock(ValidationError):
    """Raised when a transfer transaction is attempting to fulfill the
    outputs of a transaction that is in an invalid or undecided block
    """


class AssetIdMismatch(ValidationError):
    """Raised when multiple transaction inputs related to different assets"""


class AmountError(ValidationError):
    """Raised when there is a problem with a transaction's output amounts"""


class InputDoesNotExist(ValidationError):
    """Raised if a transaction input does not exist"""


class TransactionOwnerError(ValidationError):
    """Raised if a user tries to transfer a transaction they don't own"""


class DuplicateTransaction(ValidationError):
    """Raised if a duplicated transaction is found"""


class ThresholdTooDeep(ValidationError):
    """Raised if threshold condition is too deep"""


class GenesisBlockAlreadyExistsError(ValidationError):
    """Raised when trying to create the already existing genesis block"""


class MultipleValidatorOperationError(ValidationError):
    """Raised when a validator update pending but new request is submited"""
