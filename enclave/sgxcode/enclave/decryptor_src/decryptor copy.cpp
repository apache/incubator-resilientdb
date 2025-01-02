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
int ecall_dispatcher::generate_key(size_t key_size)
{
    TRACE_ENCLAVE("generating encryption key");
    int ret = 0;

    RSA *keypair = RSA_generate_key(key_size, RSA_F4, NULL, NULL);

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

    BIO_free_all(pubBIO);
    BIO_free_all(privBIO);

exit:
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


int ecall_dispatcher::decrypt(
    unsigned char *input_buf,
    unsigned char **output_buf,
    size_t input_len,
    size_t *output_len)
{
    TRACE_ENCLAVE("decrypting data");
    int ret = 0;

    RSA *priv_key = load_key_from_memory(privKeyData, false);
    *output_buf = new unsigned char[RSA_size(priv_key)];
    
    if (*output_buf == nullptr) {
        delete[] *output_buf;
        return -1;  // Memory allocation failed
    }
    
    int decrypt_length = RSA_private_decrypt(input_len, input_buf, *output_buf, priv_key, RSA_PKCS1_PADDING);

    if (decrypt_length == -1) {
        delete[] *output_buf;
        return -1;  // Encryption failed
    }

    *output_len = decrypt_length;

    return ret;  // success
}

