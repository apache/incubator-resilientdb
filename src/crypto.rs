/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

use ed25519_dalek::{Keypair, PublicKey, SecretKey};
#[allow(unused_imports)]
/// OpenSSL is a robust, open-source implementation of the SSL (Secure Sockets Layer)
/// and TLS (Transport Layer Security) protocols. It provides a toolkit for the
/// implementation of secure communication over a computer network.
use openssl::rsa::Rsa;
use sha3::{Digest, Sha3_256};

/// Hash the provided data using SHA3-256, i.e., Secure Hash Algorithm 3 using 256 bits
/// @param data Data to be hashed using SHA3-256
/// @return Hashed data
pub fn hash_data(data: &str) -> String {
    let mut hasher = Sha3_256::new();
    hasher.update(data.as_bytes());
    hex::encode(hasher.finalize())
}

/// Generates a cryptographic key pair using RSA
/// RSA is a public-key cryptosystem, meaning it uses a pair of
/// keys: a public key for encryption and a private key for decryption
/// @return tuple object containing the public and private keys
pub fn generate_keypair() -> (String, String) {
    let mut rng = rand::rngs::OsRng;
    let keypair: Keypair = Keypair::generate(&mut rng);
    let public_key: PublicKey = keypair.public;
    let secret_key: SecretKey = keypair.secret;

    let public_key_bytes: [u8; 32] = public_key.to_bytes();
    let secret_key_bytes: [u8; 32] = secret_key.to_bytes();

    // Encodes the public key bytes into a Base58 string representation.
    let public_key_base58 = bs58::encode(public_key_bytes).into_string();
    let secret_key_base58 = bs58::encode(secret_key_bytes).into_string();

    (public_key_base58, secret_key_base58)
}
