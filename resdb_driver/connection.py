import time

from collections import namedtuple
from datetime import datetime, timedelta

from requests import Session
from requests.exceptions import ConnectionError

from .exceptions import HTTP_EXCEPTIONS, TransportError


BACKOFF_DELAY = 0.5  # seconds

HttpResponse = namedtuple("HttpResponse", ("status_code", "headers", "data"))


class Connection:
    """! A Connection object to make HTTP requests to a particular node."""

    def __init__(self, *, node_url: str, headers: dict = None):
        """! Initializes a :class:`~resdb_driver.connection.Connection`
        instance.

            @param node_url (str): Url of the node to connect to.
            @param headers (dict): Optional headers to send with each request.

            @return An instance of the Connection class 
        """
        self.node_url = node_url
        self.session = Session()
        if headers:
            self.session.headers.update(headers)

        self._retries = 0
        self.backoff_time = None

    def request(
        self,
        method: str,
        *,
        path: str = None,
        json: dict = None,
        params: dict = None,
        headers: dict = None,
        timeout: int = None,
        backoff_cap: int = None,
        **kwargs
    ) -> HttpResponse:
        """! Performs an HTTP request with the given parameters. Implements exponential backoff.

        If `ConnectionError` occurs, a timestamp equal to now +
        the default delay (`BACKOFF_DELAY`) is assigned to the object.
        The timestamp is in UTC. Next time the function is called, it either
        waits till the timestamp is passed or raises `TimeoutError`.

        If `ConnectionError` occurs two or more times in a row,
        the retry count is incremented and the new timestamp is calculated
        as now + the default delay multiplied by two to the power of the
        number of retries.

        If a request is successful, the backoff timestamp is removed,
        the retry count is back to zero.

        @param method (str): HTTP method (e.g.: ``'GET'``).
        @param path (str): API endpoint path (e.g.: ``'/transactions'``).
        @param json (dict): JSON data to send along with the request.
        @param params (dict): Dictionary of URL (query) parameters.
        @param headers (dict): Optional headers to pass to the request.
        @param timeout (int): Optional timeout in seconds.
        @param backoff_cap (int): The maximal allowed backoff delay in seconds to be assigned to a node.
        @param kwargs: Optional keyword arguments.

        @return Response of the HTTP request.
        """

        backoff_timedelta = self.get_backoff_timedelta()

        if timeout is not None and timeout < backoff_timedelta:
            raise TimeoutError

        if backoff_timedelta > 0:
            time.sleep(backoff_timedelta)

        connExc = None
        timeout = timeout if timeout is None else timeout - backoff_timedelta
        try:
            response = self._request(
                method=method,
                timeout=timeout,
                url=self.node_url + path if path else self.node_url,
                json=json,
                params=params,
                headers=headers,
                **kwargs,
            )
        except ConnectionError as err:
            connExc = err
            raise err
        finally:
            self.update_backoff_time(
                success=connExc is None, backoff_cap=backoff_cap)
        return response

    def get_backoff_timedelta(self) -> float:
        if self.backoff_time is None:
            return 0

        return (self.backoff_time - datetime.utcnow()).total_seconds()

    def update_backoff_time(self, success, backoff_cap=None):
        if success:
            self._retries = 0
            self.backoff_time = None
        else:
            utcnow = datetime.utcnow()
            backoff_delta = BACKOFF_DELAY * 2**self._retries
            if backoff_cap is not None:
                backoff_delta = min(backoff_delta, backoff_cap)
            self.backoff_time = utcnow + timedelta(seconds=backoff_delta)
            self._retries += 1

    def _request(self, **kwargs) -> HttpResponse:
        response = self.session.request(**kwargs)
        text = response.text
        try:
            json = response.json()
        except ValueError:
            json = None
        if not (200 <= response.status_code < 300):
            exc_cls = HTTP_EXCEPTIONS.get(response.status_code, TransportError)
            raise exc_cls(response.status_code, text, json, kwargs["url"])
        data = json if json is not None else text
        return HttpResponse(response.status_code, response.headers, data)
