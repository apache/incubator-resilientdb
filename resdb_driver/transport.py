from time import time
from requests import request, Response

from requests.exceptions import ConnectionError

from .connection import Connection
from .exceptions import TimeoutError
from .pool import Pool


NO_TIMEOUT_BACKOFF_CAP = 10  # seconds


class Transport:
    """! Transport class.
    """

    def __init__(self, *nodes: list, timeout: int = None):
        """! Initializes an instance of
            :class:`~resdb_driver.transport.Transport`.
        @param nodes Each node is a dictionary with the keys `endpoint` and
                `headers`
        @param timeout (int): Optional timeout in seconds.
        """
        self.nodes = nodes
        self.timeout = timeout
        self.connection_pool = Pool(
            [
                Connection(node_url=node["endpoint"], headers=node["headers"])
                for node in nodes
            ]
        )

    def forward_request(
        self,
        method: str,
        path: str = None,
        json: dict = None,
        params: dict = None,
        headers: dict = None,
    ) -> Response.json:
        """! Makes HTTP requests to the configured nodes.
           Retries connection errors
           (e.g. DNS failures, refused connection, etc).
           A user may choose to retry other errors
           by catching the corresponding
           exceptions and retrying `forward_request`.
           Exponential backoff is implemented individually for each node.
           Backoff delays are expressed as timestamps stored on the object and
           they are not reset in between multiple function calls.
           Times out when `self.timeout` is expired, if not `None`.

        @param method (str): HTTP method name (e.g.: ``'GET'``).
        @param path (str): Path to be appended to the base url of a node. E.g.:
            ``'/transactions'``).
        @param json (dict): Payload to be sent with the HTTP request.
        @param params (dict): Dictionary of URL (query) parameters.
        @param headers (dict): Optional headers to pass to the request.

        @return Result of :meth:`requests.models.Response.json`

        """
        error_trace = []
        timeout = self.timeout
        backoff_cap = NO_TIMEOUT_BACKOFF_CAP if timeout is None else timeout / 2
        while timeout is None or timeout > 0:
            connection: Connection = self.connection_pool.get_connection()

            start = time()
            try:
                response = connection.request(
                    method=method,
                    path=path,
                    params=params,
                    json=json,
                    headers=headers,
                    timeout=timeout,
                    backoff_cap=backoff_cap,
                )
            except ConnectionError as err:
                error_trace.append(err)
                continue
            else:
                return response.data
            finally:
                elapsed = time() - start
                if timeout is not None:
                    timeout -= elapsed

        raise TimeoutError(error_trace)
