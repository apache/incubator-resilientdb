from urllib.parse import urlparse, urlunparse
import rapidjson
import time
import re


from .exceptions import ValidationError


def serialize(data: dict) -> str:
    """! Serialize a dict into a JSON formatted string.

    This function enforces rules like the separator and order of keys.
    This ensures that all dicts are serialized in the same way.

    This is specially important for hashing data. We need to make sure that
    everyone serializes their data in the same way so that we do not have
    hash mismatches for the same structure due to serialization
    differences.

    @param data (dict): Data to serialize

    @return JSON formatted string

    """
    return rapidjson.dumps(data, skipkeys=False, ensure_ascii=False, sort_keys=True)


def gen_timestamp():
    """! The Unix time, rounded to the nearest second.
    See https://en.wikipedia.org/wiki/Unix_time
    @return The Unix time
    """
    return str(round(time.time()))


DEFAULT_NODE = "http://localhost:9984"


class CreateOperation:
    """! Class representing the ``'CREATE'`` transaction operation.
    """


class TransferOperation:
    """! Class representing the ``'TRANSFER'`` transaction operation.
    """


ops_map = {
    "CREATE": CreateOperation,
    "TRANSFER": TransferOperation,
}


def _normalize_operation(operation):
    """! Normalizes the given operation string. For now, this simply means
    converting the given string to uppercase, looking it up in
    :attr:`~.ops_map`, and returning the corresponding class if
    present.

    @param operation (str): The operation string to convert.

    @return The class corresponding to the given string,
            :class:`~.CreateOperation` or :class:`~TransferOperation`.

        .. important:: If the :meth:`str.upper` step, or the
            :attr:`~.ops_map` lookup fails, the given ``operation``
            argument is returned.

    """
    try:
        operation = operation.upper()
    except AttributeError:
        pass

    try:
        operation = ops_map[operation]()
    except KeyError:
        pass

    return operation


def _get_default_port(scheme):
    return 443 if scheme == "https" else 9984


def normalize_url(node):
    """! Normalizes the given node url"""
    if not node:
        node = DEFAULT_NODE
    elif "://" not in node:
        node = "//{}".format(node)
    parts = urlparse(node, scheme="http", allow_fragments=False)
    port = parts.port if parts.port else _get_default_port(parts.scheme)
    netloc = "{}:{}".format(parts.hostname, port)
    return urlunparse((parts.scheme, netloc, parts.path, "", "", ""))


def normalize_node(node, headers=None):
    """! Normalizes given node as str or dict with headers"""
    headers = {} if headers is None else headers
    if isinstance(node, str):
        url = normalize_url(node)
        return {"endpoint": url, "headers": headers}

    url = normalize_url(node["endpoint"])
    node_headers = node.get("headers", {})
    return {"endpoint": url, "headers": {**headers, **node_headers}}


def normalize_nodes(*nodes, headers=None):
    """! Normalizes given dict or array of driver nodes"""
    if not nodes:
        return (normalize_node(DEFAULT_NODE, headers),)

    normalized_nodes = ()
    for node in nodes:
        normalized_nodes += (normalize_node(node, headers),)
    return normalized_nodes
