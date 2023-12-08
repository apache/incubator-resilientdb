 #[allow(unused_imports)]
 /// OpenSSL is a robust, open-source implementation of the SSL (Secure Sockets Layer)
 /// and TLS (Transport Layer Security) protocols. It provides a toolkit for the
 /// implementation of secure communication over a computer network. 
use openssl::rsa::Rsa;
use openssl::pkey::PKey;
use openssl::pkey::Private;
use sha3::{Digest, Sha3_256};
use std::convert::TryInto;

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
/// @param key-size
/// @return tuple object containing the public and private keys
pub fn generate_keypair(key_size: usize) -> (Vec<u8>, PKey<Private>) {
    //let rsa = Rsa::generate(key_size).unwrap();
    let rsa = Rsa::generate(key_size.try_into().unwrap()).unwrap();
    let pkey = PKey::from_rsa(rsa).unwrap();
    let pub_key: Vec<u8> = pkey.public_key_to_pem().unwrap();
    (pub_key, pkey)
}
