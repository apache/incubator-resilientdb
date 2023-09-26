from crypt import methods
from curses import meta
from .transport import Transport
from .offchain import prepare_transaction, fulfill_transaction
from .utils import normalize_nodes
from typing import Any, Union


class Resdb:
    """! A :class:`~resdb_driver.Resdb` driver is able to create, sign,
    and submit transactions to one or more nodes in a Federation.

    If initialized with ``>1`` nodes, the driver will send successive
    requests to different nodes in a round-robin fashion (this will be
    customizable in the future).

    """

    def __init__(
        self,
        *nodes: list[Union[str, dict]],
        transport_class: Transport = Transport,
        headers=None,
        timeout=20
    ):
        """! Initialize a :class:`~resdb_driver.Resdb` driver instance.

        @param *nodes (list of (str or dict)): Resdb nodes to connect to.
                Currently, the full URL must be given. In the absence of any
                node, the default(``'http://localhost:9984'``) will be used.
                If node is passed as a dict, `endpoint` is a required key;
                `headers` is an optional `dict` of headers.
        @param transport_class Optional transport class to use.
                Defaults to :class:`~resdb_driver.transport.Transport`.
        @param headers (dict): Optional headers that will be passed with
                each request. To pass headers only on a per-request
                basis, you can pass the headers to the method of choice
                (e.g. :meth:`Resdb().transactions.send_commit()
                <.TransactionsEndpoint.send_commit>`).
        @param timeout (int): Optional timeout in seconds that will be passed
                to each request.

        @return An instance of the Resdb class
        """
        self._nodes = normalize_nodes(*nodes, headers=headers)
        self._transport = transport_class(*self._nodes, timeout=timeout)
        self._transactions = TransactionsEndpoint(self)
        self._outputs = OutputsEndpoint(self)
        self._blocks = BlocksEndpoint(self)
        self._assets = AssetsEndpoint(self)
        self._metadata = MetadataEndpoint(self)
        self.api_prefix = "/v1"

    @property
    def nodes(self):
        """! :obj:`tuple` of :obj:`str`: URLs of connected nodes.
        """
        return self._nodes

    @property
    def transport(self):
        """! :class:`~resdb_driver.transport.Transport`: Object
        responsible for forwarding requests to a
        :class:`~resdb_driver.connection.Connection` instance (node).
        """
        return self._transport

    @property
    def transactions(self):
        """! :class:`~resdb_driver.driver.TransactionsEndpoint`:
        Exposes functionalities of the ``'/transactions'`` endpoint.
        TODO: check
        """
        return self._transactions

    @property
    def outputs(self):
        """! :class:`~resdb_driver.driver.OutputsEndpoint`:
        Exposes functionalities of the ``'/outputs'`` endpoint.
        TODO: check
        """
        return self._outputs

    @property
    def assets(self):
        """! :class:`~resdb_driver.driver.AssetsEndpoint`:
        Exposes functionalities of the ``'/assets'`` endpoint.
        TODO: check
        """
        return self._assets

    @property
    def metadata(self):
        """! :class:`~resdb_driver.driver.MetadataEndpoint`:
        Exposes functionalities of the ``'/metadata'`` endpoint.
        TODO: check 
        """
        return self._metadata

    @property
    def blocks(self):
        """! :class:`~resdb_driver.driver.BlocksEndpoint`:
        Exposes functionalities of the ``'/blocks'`` endpoint.
        TODO: check
        """
        return self._blocks

    def info(self, headers=None) -> dict:
        """! Retrieves information of the node being connected to via the
        root endpoint ``'/'``.

        Note:
            TODO: implement the endpoint in the node (Single node)

        @param headers (dict): Optional headers to pass to the request.

        @return Details of the node that this instance is connected
        to. Some information that may be interesting - Server version, overview of all the endpoints
        """

        return self.transport.forward_request(method="GET", path="/", headers=headers)

    def api_info(self, headers=None) -> dict:
        """! Retrieves information provided by the API root endpoint
        ``'/api/v1'``.

        TODO: implement the endpoint in the node 

        @param headers (dict): Optional headers to pass to the request.

        @return Details of the HTTP API provided by the Resdb
        server.
        """
        return self.transport.forward_request(
            method="GET",
            path=self.api_prefix,
            headers=headers,
        )

    def get_transaction(self, txid):
        # TODO
        # use transactions.retieve() instead
        # return self._transactions.retrieve(txid=txid)
        raise NotImplementedError


class NamespacedDriver:
    """! Base class for creating endpoints (namespaced objects) that can be added
    under the :class:`~resdb_driver.driver.Resdb` driver.
    """

    PATH = "/"

    def __init__(self, driver: Resdb):
        """! Initializes an instance of
            :class:`~resdb_driver.driver.NamespacedDriver` with the given
            driver instance.

        @param driver (Resdb): Instance of :class:`~resdb_driver.driver.Resdb`.

        @return An instance of the NamespacedDriver class.
        """
        self.driver = driver

    @property
    def transport(self) -> Transport:
        return self.driver.transport

    @property
    def api_prefix(self) -> str:
        return self.driver.api_prefix

    @property
    def path(self) -> str:
        return self.api_prefix + self.PATH


class TransactionsEndpoint(NamespacedDriver):
    """! Exposes functionality of the ``'/transactions/'`` endpoint.

    Attributes:
        path (str): The path of the endpoint.

    """

    PATH = "/transactions/"

    @staticmethod
    def prepare(
        *,
        operation="CREATE",
        signers=None,
        recipients=None,
        asset=None,
        metadata=None,
        inputs=None
    ) -> dict:
        """! Prepares a transaction payload, ready to be fulfilled.

        @param operation (str): The operation to perform. Must be ``'CREATE'``
                or ``'TRANSFER'``. Case insensitive. Defaults to ``'CREATE'``.
        @param signers (:obj:`list` | :obj:`tuple` | :obj:`str`, optional): One or more public keys representing the issuer(s) of
                the asset being created. Only applies for ``'CREATE'``
                operations. Defaults to ``None``.
        @param recipients (:obj:`list` | :obj:`tuple` | :obj:`str`, optional): One or more public keys representing the new recipients(s)
                of the asset being created or transferred. Defaults to ``None``.
        @param asset (:obj:`list` | :obj:`tuple` | :obj:`str`, optional): The asset to be created or transferred. MUST be supplied 
                for ``'TRANSFER'`` operations. Defaults to ``None``.
        @param metadata (:obj:`list` | :obj:`tuple` | :obj:`str`, optional): Metadata associated with the transaction. Defaults to ``None``.
        @param inputs (:obj:`dict` | :obj:`list` | :obj:`tuple`, optional): One or more inputs holding the condition(s) that this
                transaction intends to fulfill. Each input is expected to
                be a :obj:`dict`. Only applies to, and MUST be supplied for,
                ``'TRANSFER'`` operations.

        @return The prepared transaction (dict)

        @exception :class:`~.exceptions.ResdbException`: If ``operation`` is
            not ``'CREATE'`` or ``'TRANSFER'``.

        .. important::

            **CREATE operations**

            * ``signers`` MUST be set.
            * ``recipients``, ``asset``, and ``metadata`` MAY be set.
            * If ``asset`` is set, it MUST be in the form of::

                {
                    'data': {
                        ...
                    }
                }

            * The argument ``inputs`` is ignored.
            * If ``recipients`` is not given, or evaluates to
              ``False``, it will be set equal to ``signers``::

                if not recipients:
                    recipients = signers

            **TRANSFER operations**

            * ``recipients``, ``asset``, and ``inputs`` MUST be set.
            * ``asset`` MUST be in the form of::

                {
                    'id': '<Asset ID (i.e. TX ID of its CREATE transaction)>'
                }

            * ``metadata`` MAY be set.
            * The argument ``signers`` is ignored.

        """
        return prepare_transaction(
            operation=operation,
            signers=signers,
            recipients=recipients,
            asset=asset,
            metadata=metadata,
            inputs=inputs,
        )

    @staticmethod
    def fulfill(
        transaction: dict, private_keys: Union[str, list, tuple]
    ) -> dict[str, Any]:
        """! Fulfills the given transaction.

        @param transaction (dict): The transaction to be fulfilled.
        @param private_keys (:obj:`str` | :obj:`list` | :obj:`tuple`): One or more private keys to be 
                used for fulfilling the transaction.

        @return The fulfilled transaction payload, ready to 
                be sent to a Resdb federation.

        @exception :exc:`~.exceptions.MissingPrivateKeyError`: If a private
                key is missing.
        """
        return fulfill_transaction(transaction, private_keys=private_keys)

    def get(self, *, asset_id, operation=None, headers=None) -> list:
        """! Given an asset id, get its list of transactions (and
        optionally filter for only ``'CREATE'`` or ``'TRANSFER'``
        transactions).

        Note:
            Please note that the id of an asset in Resdb is
            actually the id of the transaction which created the asset.
            In other words, when querying for an asset id with the
            operation set to ``'CREATE'``, only one transaction should
            be expected. This transaction will be the transaction in
            which the asset was created, and the transaction id will be
            equal to the given asset id. Hence, the following calls to
            :meth:`.retrieve` and :meth:`.get` should return the same
            transaction.

                >>> resdb = Resdb()
                >>> resdb.transactions.retrieve('foo')
                >>> resdb.transactions.get(asset_id='foo', operation='CREATE')

            Since :meth:`.get` returns a list of transactions, it may
            be more efficient to use :meth:`.retrieve` instead, if one
            is only interested in the ``'CREATE'`` operation.

        @param asset_id (str): Id of the asset.
        @param operation (str): The type of operation the transaction
                should be. Either ``'CREATE'`` or ``'TRANSFER'``.
                Defaults to ``None``.
        @param headers (dict): Optional headers to pass to the request.

        @return List of transactions
        """

        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"asset_id": asset_id, "operation": operation},
            headers=headers,
        )

    def send_async(self, transaction, headers=None):
        """!
        Note:
            Not used in resdb
        Submit a transaction to the Federation with the mode `async`.

            @param transaction (dict): The transaction to be sent
                to the Federation node(s).
            @param headers (dict): Optional headers to pass to the request.

            @return The transaction sent to the Federation node(s).

        """
        # return self.transport.forward_request(
        #     method="POST",
        #     path=self.path,
        #     json=transaction,
        #     params={"mode": "async"},
        #     headers=headers,
        # )
        raise NotImplementedError

    def send_sync(self, transaction, headers=None):
        """! Note:
        Not used in resdb
        Submit a transaction to the Federation with the mode `sync`.

        @param transaction (dict): The transaction to be sent
            to the Federation node(s).
        @param headers (dict): Optional headers to pass to the request.

        @return The transaction sent to the Federation node(s).

        """
        # return self.transport.forward_request(
        #     method="POST",
        #     path=self.path,
        #     json=transaction,
        #     params={"mode": "sync"},
        #     headers=headers,
        # )
        raise NotImplementedError

    def send_commit(self, transaction: dict, headers: dict = None) -> dict:
        """! Submit a transaction to the Federation with the mode `commit`.

        @param transaction (dict): The transaction to be sent
            to the Federation node(s).
        @param headers (dict): Optional headers to pass to the request.

        @return The transaction sent to the Federation node(s).
        """
        # return self.transport.forward_request(
        #     method='POST',
        #     path=self.path,
        #     json=transaction,
        #     params={'mode': 'commit'},
        #     headers=headers)
        function = "commit"
        path = self.path + function
        return self.transport.forward_request(
            method="POST", path=path, json=transaction, headers=headers
        )

    def retrieve(self, txid: str, headers: dict = None) -> dict:
        """! Retrieves the transaction with the given id.

        @param txid (str): Id of the transaction to retrieve.
        @param headers (dict): Optional headers to pass to the request.

        @return The transaction with the given id.
        """
        path = self.path + txid
        return self.transport.forward_request(method="GET", path=path, headers=None)


class OutputsEndpoint(NamespacedDriver):
    """! TODO:
    add endpoint in nodes
    Exposes functionality of the ``'/outputs'`` endpoint.

    path (str): The path of the endpoint.
    """

    PATH = "/outputs/"

    def get(self, public_key, spent=None, headers=None) -> list[dict]:
        """! Get transaction outputs by public key. The public_key parameter
        must be a base58 encoded ed25519 public key associated with
        transaction output ownership.

        Example:
            Given a transaction with `id` ``da1b64a907ba54`` having an
            `ed25519` condition (at index ``0``) with alice's public
            key::

                >>> resdb = Resdb()
                >>> resdb.outputs.get(alice_pubkey)
                ... ['../transactions/da1b64a907ba54/conditions/0']

        @param public_key (str): Public key for which unfulfilled
            conditions are sought.
        @param spent (bool): Indicate if the result set should include only spent
            or only unspent outputs. If not specified (``None``) the
            result includes all the outputs (both spent and unspent)
            associated with the public key.
        @param headers (dict): Optional headers to pass to the request.

        @return List of unfulfilled conditions.
        """

        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"public_key": public_key, "spent": spent},
            headers=headers,
        )


class BlocksEndpoint(NamespacedDriver):
    """! Exposes functionality of the ``'/blocks'`` endpoint.

    Attributes:
        path (str): The path of the endpoint.

    """

    PATH = "/blocks/"

    def get(self, *, txid, headers=None) -> list[dict]:
        """! TODO:
        Add endpoints in nodes. transaction_id is a query parameter here
        Get the block that contains the given transaction id (``txid``)
        else return ``None``

        @param txid (str): Transaction id.
        @param headers (dict): Optional headers to pass to the request.

        @return List of block heights.

        """
        block_list = self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"transaction_id": txid},
            headers=headers,
        )
        return block_list[0] if len(block_list) else None


class AssetsEndpoint(NamespacedDriver):
    """! Exposes functionality of the ``'/assets'`` endpoint.

        path (str): The path of the endpoint.
    """

    PATH = "/assets/"

    def get(self, *, search, limit=0, headers=None) -> list[dict]:
        """! TODO:
        add endpoints in nodes. transaction_id is a query parameter here
        Retrieves the assets that match a given text search string.

        @param search (str): Text search string.
        @param limit (int): Limit the number of returned documents. Defaults to
            zero meaning that it returns all the matching assets.
        @param headers (dict): Optional headers to pass to the request.

        @return List of assets that match the query.
        """

        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"search": search, "limit": limit},
            headers=headers,
        )


class MetadataEndpoint(NamespacedDriver):
    """! Exposes functionality of the ``'/metadata'`` endpoint.

    path (str): The path of the endpoint.

    """

    PATH = "/metadata/"

    def get(self, *, search, limit=0, headers=None) -> list[dict]:
        """! Retrieves the metadata that match a given text search string.

        @param search (str): Text search string.
        @param limit (int): Limit the number of returned documents. Defaults to
            zero meaning that it returns all the matching metadata.
        @param headers (dict): Optional headers to pass to the request.

        @return List of metadata that match the query.

        """
        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"search": search, "limit": limit},
            headers=headers,
        )
