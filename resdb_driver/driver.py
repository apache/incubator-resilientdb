from crypt import methods
from curses import meta
from .transport import Transport
from .offchain import prepare_transaction, fulfill_transaction
from .utils import normalize_nodes
from typing import Any, Union


class Resdb:
    """A :class:`~resdb_driver.Resdb` driver is able to create, sign,
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
        """Initialize a :class:`~resdb_driver.Resdb` driver instance.

        Args:
            *nodes (list of (str or dict)): Resdb nodes to connect to.
                Currently, the full URL must be given. In the absence of any
                node, the default(``'http://localhost:9984'``) will be used.
                If node is passed as a dict, `endpoint` is a required key;
                `headers` is an optional `dict` of headers.
            transport_class: Optional transport class to use.
                Defaults to :class:`~resdb_driver.transport.Transport`.
            headers (dict): Optional headers that will be passed with
                each request. To pass headers only on a per-request
                basis, you can pass the headers to the method of choice
                (e.g. :meth:`Resdb().transactions.send_commit()
                <.TransactionsEndpoint.send_commit>`).
            timeout (int): Optional timeout in seconds that will be passed
                to each request.
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
        """:obj:`tuple` of :obj:`str`: URLs of connected nodes."""
        return self._nodes

    @property
    def transport(self):
        """:class:`~resdb_driver.transport.Transport`: Object
        responsible for forwarding requests to a
        :class:`~resdb_driver.connection.Connection` instance (node).
        """
        return self._transport

    @property
    def transactions(self):
        """:class:`~resdb_driver.driver.TransactionsEndpoint`:
        Exposes functionalities of the ``'/transactions'`` endpoint.
        """
        return self._transactions

    @property
    def outputs(self):
        """:class:`~resdb_driver.driver.OutputsEndpoint`:
        Exposes functionalities of the ``'/outputs'`` endpoint.
        """
        return self._outputs

    @property
    def assets(self):
        """:class:`~resdb_driver.driver.AssetsEndpoint`:
        Exposes functionalities of the ``'/assets'`` endpoint.
        """
        return self._assets

    @property
    def metadata(self):
        """:class:`~resdb_driver.driver.MetadataEndpoint`:
        Exposes functionalities of the ``'/metadata'`` endpoint.
        """
        return self._metadata

    @property
    def blocks(self):
        """:class:`~resdb_driver.driver.BlocksEndpoint`:
        Exposes functionalities of the ``'/blocks'`` endpoint.
        """
        return self._blocks

    def info(self, headers=None):
        """Retrieves information of the node being connected to via the
        root endpoint ``'/'``.

        Args:
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: Details of the node that this instance is connected
            to. Some information that may be interesting:

                * the server version and
                * an overview of all the endpoints

        Note:
            Currently limited to one node, and will be expanded to
            return information for each node that this instance is
            connected to.

        """
        return self.transport.forward_request(method="GET", path="/", headers=headers)

    def api_info(self, headers=None):
        """Retrieves information provided by the API root endpoint
        ``'/api/v1'``.

        Args:
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: Details of the HTTP API provided by the Resdb
            server.

        """
        return self.transport.forward_request(
            method="GET",
            path=self.api_prefix,
            headers=headers,
        )

    def get_transaction(self, txid):
        # TODO
        raise NotImplementedError


class NamespacedDriver:
    """Base class for creating endpoints (namespaced objects) that can be added
    under the :class:`~resdb_driver.driver.Resdb` driver.
    """

    PATH = "/"

    def __init__(self, driver: Resdb):
        """Initializes an instance of
        :class:`~resdb_driver.driver.NamespacedDriver` with the given
        driver instance.

        Args:
            driver (Resdb): Instance of
                :class:`~resdb_driver.driver.Resdb`.
        """
        self.driver = driver

    @property
    def transport(self):
        return self.driver.transport

    @property
    def api_prefix(self):
        return self.driver.api_prefix

    @property
    def path(self):
        return self.api_prefix + self.PATH


class TransactionsEndpoint(NamespacedDriver):
    """Exposes functionality of the ``'/transactions/'`` endpoint.

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
    ):
        """Prepares a transaction payload, ready to be fulfilled.

        Args:
            operation (str): The operation to perform. Must be ``'CREATE'``
                or ``'TRANSFER'``. Case insensitive. Defaults to ``'CREATE'``.
            signers (:obj:`list` | :obj:`tuple` | :obj:`str`, optional):
                One or more public keys representing the issuer(s) of
                the asset being created. Only applies for ``'CREATE'``
                operations. Defaults to ``None``.
            recipients (:obj:`list` | :obj:`tuple` | :obj:`str`, optional):
                One or more public keys representing the new recipients(s)
                of the asset being created or transferred.
                Defaults to ``None``.
            asset (:obj:`dict`, optional): The asset to be created or
                transferred. MUST be supplied for ``'TRANSFER'`` operations.
                Defaults to ``None``.
            metadata (:obj:`dict`, optional): Metadata associated with the
                transaction. Defaults to ``None``.
            inputs (:obj:`dict` | :obj:`list` | :obj:`tuple`, optional):
                One or more inputs holding the condition(s) that this
                transaction intends to fulfill. Each input is expected to
                be a :obj:`dict`. Only applies to, and MUST be supplied for,
                ``'TRANSFER'`` operations.

        Returns:
            dict: The prepared transaction.

        Raises:
            :class:`~.exceptions.ResdbException`: If ``operation`` is
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
        """Fulfills the given transaction.

        Args:
            transaction (dict): The transaction to be fulfilled.
            private_keys (:obj:`str` | :obj:`list` | :obj:`tuple`): One or
                more private keys to be used for fulfilling the
                transaction.

        Returns:
            dict: The fulfilled transaction payload, ready to be sent to a
            Resdb federation.

        Raises:
            :exc:`~.exceptions.MissingPrivateKeyError`: If a private
                key is missing.

        """
        return fulfill_transaction(transaction, private_keys=private_keys)

    def get(self, *, asset_id, operation=None, headers=None):
        """Given an asset id, get its list of transactions (and
        optionally filter for only ``'CREATE'`` or ``'TRANSFER'``
        transactions).

        Args:
            asset_id (str): Id of the asset.
            operation (str): The type of operation the transaction
                should be. Either ``'CREATE'`` or ``'TRANSFER'``.
                Defaults to ``None``.
            headers (dict): Optional headers to pass to the request.

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

        Returns:
            list: List of transactions.

        """
        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"asset_id": asset_id, "operation": operation},
            headers=headers,
        )

    def send_async(self, transaction, headers=None):
        """Submit a transaction to the Federation with the mode `async`.

        Args:
            transaction (dict): the transaction to be sent
                to the Federation node(s).
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: The transaction sent to the Federation node(s).

        """
        return self.transport.forward_request(
            method="POST",
            path=self.path,
            json=transaction,
            params={"mode": "async"},
            headers=headers,
        )

    def send_sync(self, transaction, headers=None):
        """Submit a transaction to the Federation with the mode `sync`.

        Args:
            transaction (dict): the transaction to be sent
                to the Federation node(s).
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: The transaction sent to the Federation node(s).

        """
        return self.transport.forward_request(
            method="POST",
            path=self.path,
            json=transaction,
            params={"mode": "sync"},
            headers=headers,
        )

    def send_commit(self, transaction: dict, headers: dict = None) -> dict:
        """Submit a transaction to the Federation with the mode `commit`.

        Args:
            transaction (dict): the transaction to be sent
                to the Federation node(s).
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: The transaction sent to the Federation node(s).

        """
        # return self.transport.forward_request(
        #     method='POST',
        #     path=self.path,
        #     json=transaction,
        #     params={'mode': 'commit'},
        #     headers=headers)
        path = self.path + "commit"
        return self.transport.forward_request(
            method="POST", path=path, json=transaction, headers=headers
        )

    def retrieve(self, txid: str, headers: dict = None) -> dict:
        """Retrieves the transaction with the given id.

        Args:
            txid (str): Id of the transaction to retrieve.
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: The transaction with the given id.

        """
        path = self.path + txid
        return self.transport.forward_request(method="GET", path=path, headers=None)


class OutputsEndpoint(NamespacedDriver):
    """Exposes functionality of the ``'/outputs'`` endpoint.

    Attributes:
        path (str): The path of the endpoint.

    """

    PATH = "/outputs/"

    def get(self, public_key, spent=None, headers=None):
        """Get transaction outputs by public key. The public_key parameter
        must be a base58 encoded ed25519 public key associated with
        transaction output ownership.

        Args:
            public_key (str): Public key for which unfulfilled
                conditions are sought.
            spent (bool): Indicate if the result set should include only spent
                or only unspent outputs. If not specified (``None``) the
                result includes all the outputs (both spent and unspent)
                associated with the public key.
            headers (dict): Optional headers to pass to the request.

        Returns:
            :obj:`list` of :obj:`str`: List of unfulfilled conditions.

        Example:
            Given a transaction with `id` ``da1b64a907ba54`` having an
            `ed25519` condition (at index ``0``) with alice's public
            key::

                >>> resdb = Resdb()
                >>> resdb.outputs.get(alice_pubkey)
                ... ['../transactions/da1b64a907ba54/conditions/0']

        """
        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"public_key": public_key, "spent": spent},
            headers=headers,
        )


class BlocksEndpoint(NamespacedDriver):
    """Exposes functionality of the ``'/blocks'`` endpoint.

    Attributes:
        path (str): The path of the endpoint.

    """

    PATH = "/blocks/"

    def get(self, *, txid, headers=None):
        """Get the block that contains the given transaction id (``txid``)
           else return ``None``

        Args:
            txid (str): Transaction id.
            headers (dict): Optional headers to pass to the request.

        Returns:
            :obj:`list` of :obj:`int`: List of block heights.

        """
        block_list = self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"transaction_id": txid},
            headers=headers,
        )
        return block_list[0] if len(block_list) else None

    def retrieve(self, block_height, headers=None):
        """Retrieves the block with the given ``block_height``.

        Args:
            block_height (str): height of the block to retrieve.
            headers (dict): Optional headers to pass to the request.

        Returns:
            dict: The block with the given ``block_height``.

        """
        path = self.path + block_height
        return self.transport.forward_request(method="GET", path=path, headers=None)


class AssetsEndpoint(NamespacedDriver):
    """Exposes functionality of the ``'/assets'`` endpoint.

    Attributes:
        path (str): The path of the endpoint.

    """

    PATH = "/assets/"

    def get(self, *, search, limit=0, headers=None):
        """Retrieves the assets that match a given text search string.

        Args:
            search (str): Text search string.
            limit (int): Limit the number of returned documents. Defaults to
                zero meaning that it returns all the matching assets.
            headers (dict): Optional headers to pass to the request.

        Returns:
            :obj:`list` of :obj:`dict`: List of assets that match the query.

        """
        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"search": search, "limit": limit},
            headers=headers,
        )


class MetadataEndpoint(NamespacedDriver):
    """Exposes functionality of the ``'/metadata'`` endpoint.

    Attributes:
        path (str): The path of the endpoint.

    """

    PATH = "/metadata/"

    def get(self, *, search, limit=0, headers=None):
        """Retrieves the metadata that match a given text search string.

        Args:
            search (str): Text search string.
            limit (int): Limit the number of returned documents. Defaults to
                zero meaning that it returns all the matching metadata.
            headers (dict): Optional headers to pass to the request.

        Returns:
            :obj:`list` of :obj:`dict`: List of metadata that match the query.

        """
        return self.transport.forward_request(
            method="GET",
            path=self.path,
            params={"search": search, "limit": limit},
            headers=headers,
        )
