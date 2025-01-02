#include "common/dispatcher.h"
#include "common/trace.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <cstring>
#include <iostream>
#include <iomanip>

using namespace std;

// Generate an encryption key: this is the key used to encrypt data
int ecall_dispatcher::generate_key(size_t* key_size)
{
    TRACE_ENCLAVE("generating encryption key 1");
    ERR_load_crypto_strings();
    int ret = 0;

/*
    RSA *keypair = RSA_generate_key(*key_size, RSA_F4, NULL, NULL);

    BIO *pubBIO = BIO_new(BIO_s_mem());
    BIO *privBIO = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPublicKey(pubBIO, keypair);
    PEM_write_bio_RSAPrivateKey(privBIO, keypair, NULL, NULL, 0, NULL, NULL);

    pubLen = BIO_pending(pubBIO);
    privLen = BIO_pending(privBIO);

    pubKeyData = new unsigned char[pubLen + 1];
    privKeyData = new unsigned char[privLen + 1];

    BIO_read(pubBIO, pubKeyData, pubLen);
    BIO_read(privBIO, privKeyData, privLen);

    pubKeyData[pubLen] = '\0';
    privKeyData[privLen] = '\0';
    
    pubLen = BIO_pending(pubBIO);
    privLen = BIO_pending(privBIO);

    BIO_free_all(pubBIO);
    BIO_free_all(privBIO);
*/

{
    size_t pubLen_ = 426;
    size_t privLen_ = 1679;
    std::string pubKeyDataStr = R"(-----BEGIN RSA PUBLIC KEY-----
sample public key
-----END RSA PUBLIC KEY-----)";
    std::string privKeyDataStr = R"(-----BEGIN RSA PRIVATE KEY-----
sample private key
-----END RSA PRIVATE KEY-----)";

    pubKeyData = new unsigned char[pubKeyDataStr.size() + 1];
    privKeyData = new unsigned char[privKeyDataStr.size() + 1];
    
    std::memcpy(pubKeyData, pubKeyDataStr.c_str(), pubKeyDataStr.size() + 1); // 包括 null terminator
    std::memcpy(privKeyData, privKeyDataStr.c_str(), privKeyDataStr.size() + 1);

    pubLen = pubKeyDataStr.size() + 1;
    privLen = privKeyDataStr.size() + 1;
}

    TRACE_ENCLAVE("pubKeyData: \n%s",pubKeyData);
    TRACE_ENCLAVE("privKeyData: \n%s",privKeyData);
    TRACE_ENCLAVE("pubLen: %d",pubLen);
    TRACE_ENCLAVE("privLen: %d",privLen);

    return ret;
}

int ecall_dispatcher::update_key(
    unsigned char* priv_key,
    size_t priv_len,
    unsigned char* pub_key,
    size_t pub_len)
{
    delete[] privKeyData;
    privKeyData = new unsigned char[priv_len + 1];
    privLen = priv_len;
    memcpy(privKeyData, priv_key, priv_len);
    
    delete[] pubKeyData;
    pubKeyData = new unsigned char[pub_len + 1];
    pubLen = pub_len;
    memcpy(pubKeyData, pub_key, pub_len);
}

RSA *load_key_from_memory(unsigned char *keydata, bool public_key)
{
    BIO *bio = BIO_new_mem_buf(keydata, -1);
    RSA *key = NULL;
    if (public_key)
    {
        PEM_read_bio_RSAPublicKey(bio, &key, NULL, NULL);
    }
    else
    {
        PEM_read_bio_RSAPrivateKey(bio, &key, NULL, NULL);
    }

    BIO_free_all(bio);
    return key;
}

int ecall_dispatcher::get_pubkey(
    unsigned char **key_buf,
    size_t *key_len)
{
    TRACE_ENCLAVE("get_pubkey: \n%s",pubKeyData);
    int ret = 0;
    *key_buf = new unsigned char[pubLen + 1];
    std::memcpy(*key_buf, pubKeyData, pubLen);
    (*key_buf)[pubLen] = '\0';
    *key_len = pubLen;
    return ret;
}


int ecall_dispatcher::encrypt(
    unsigned char *input_buf,
    unsigned char **output_buf,
    size_t input_len,
    size_t *output_len)
{
    TRACE_ENCLAVE("encrypting data");
    int ret = 0;

    RSA *pub_key = load_key_from_memory(pubKeyData, true);
    *output_buf = new unsigned char[RSA_size(pub_key)];
    
    if (*output_buf == nullptr) {
        delete[] *output_buf;
        return -1;  // Memory allocation failed
    }

    int encrypt_length = RSA_public_encrypt(input_len, input_buf, *output_buf, pub_key, RSA_PKCS1_PADDING);

    if (encrypt_length == -1) {
        TRACE_ENCLAVE("encrypting data failed");
        delete[] *output_buf;
        return -1;  // Encryption failed
    }

    *output_len = encrypt_length;

    return ret;  // success
}


// int ecall_dispatcher::decrypt(
//     unsigned char *input_buf,
//     unsigned char **output_buf,
//     size_t input_len,
//     size_t *output_len)
// {
//     int ret = 0;

//     RSA *priv_key = load_key_from_memory(privKeyData, false);
//     *output_buf = new unsigned char[RSA_size(priv_key)];
    
//     if (*output_buf == nullptr) {
//         delete[] *output_buf;
//         return -1;  // Memory allocation failed
//     }
    
//     int decrypt_length = RSA_private_decrypt(input_len, input_buf, *output_buf, priv_key, RSA_PKCS1_PADDING);

//     if (decrypt_length == -1) {
//         delete[] *output_buf;
//         return -1;  // Encryption failed
//     }

//     *output_len = decrypt_length;
    
//     return ret;  // succeed
// }

int ecall_dispatcher::decrypt(
    unsigned char *input_buf,
    unsigned char **output_buf,
    size_t input_len,
    size_t *output_len)
{
    int ret = 0;

    RSA *priv_key = load_key_from_memory(privKeyData, false);
    if (priv_key == nullptr) {
        TRACE_ENCLAVE("Failed to load private key.");
        return -1;
    }

    int key_size = RSA_size(priv_key);
    *output_buf = new unsigned char[key_size];
    if (*output_buf == nullptr) {
        TRACE_ENCLAVE("Memory allocation failed.");
        RSA_free(priv_key);
        return -1;
    }

    int decrypt_length = RSA_private_decrypt(
        input_len,
        input_buf,
        *output_buf,
        priv_key,
        RSA_PKCS1_PADDING);

    if (decrypt_length == -1) {
        unsigned long err = ERR_get_error();
        char err_msg[120];
        ERR_error_string(err, err_msg);
        TRACE_ENCLAVE("Decryption failed: %s",err_msg);
        delete[] *output_buf;
        RSA_free(priv_key);
        return -1;  // Decryption failed
    }

    *output_len = decrypt_length;
    RSA_free(priv_key);
    return ret;  // Success
}
