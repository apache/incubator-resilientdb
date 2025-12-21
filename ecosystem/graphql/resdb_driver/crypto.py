# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.    



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
