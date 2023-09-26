from collections import namedtuple
from cryptoconditions import crypto

import sha3

CryptoKeypair = namedtuple("CryptoKeypair", ("private_key", "public_key"))


def generate_keypair(seed=None) -> CryptoKeypair:
    """! Generates a cryptographic key pair.

    @param seed (bytes): 32-byte seed for deterministic generation. Defaults to `None`.

    @return collections.namedtuple object containing the public and private keys.
    """

    return CryptoKeypair(*(k.decode() for k in crypto.ed25519_generate_key_pair(seed)))


def hash_data(data) -> sha3.sha3_256:
    """! Hash the provided data using SHA3-256

    @param data Data to be hashed using SHA3-256

    @return Hashed data
    """
    # print(f"HASH DATA {type(data)}, {data=}")
    return sha3.sha3_256(data.encode()).hexdigest()


PrivateKey = crypto.Ed25519SigningKey
PublicKey = crypto.Ed25519VerifyingKey
