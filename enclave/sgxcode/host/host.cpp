// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

// Note: Only for testing enclave !

#include <iostream>
#include <iomanip>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "sgx_cpp_u.h"

using namespace std;

#define CIPHER_BLOCK_SIZE 16
#define DATA_BLOCK_SIZE 256
#define ENCRYPT_OPERATION true
#define DECRYPT_OPERATION false

oe_enclave_t *enclave = NULL;

bool check_simulate_opt(int *argc, const char *argv[])
{
    for (int i = 0; i < *argc; i++)
    {
        if (strcmp(argv[i], "--simulate") == 0)
        {
            cout << "Running in simulation mode" << endl;
            memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char *));
            (*argc)--;
            return true;
        }
    }
    return false;
}

void printHex(unsigned char* data, size_t length) {
    cout << "{";
    for (size_t i = 0; i < length; ++i) {
        printf("0x%02x ", data[i]);
    }
    ::cout << "}" << endl;
}


int main(int argc, const char *argv[])
{
    oe_result_t result;
    int ret = 0;
    uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;

    if (check_simulate_opt(&argc, argv))
    {
        flags |= OE_ENCLAVE_FLAG_SIMULATE;
    }

    cout << "Host: enter main" << endl;
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0]
             << " enclave_image_path [ --simulate  ]" << endl;
        return 1;
    }

    cout << "Host: create enclave for image:" << argv[1] << endl;
    result = oe_create_sgx_cpp_enclave(
        argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave);
    if (result != OE_OK)
    {
        cerr << "oe_create_sgx_cpp_enclave() failed with " << argv[0]
             << " " << result << endl;
        ret = 1;
        // goto exit;
    }

    // counter test
    {
        // request a counter
        cout << "Host: requesting a counter:" << endl;
        uint32_t index = -1;
        result = request_counter(enclave, &ret, &index);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: request_counter failed with " << ret << endl;
            // goto exit;
        }

        // request a counter
        cout << "Host: requesting a counter:" << endl;
        uint32_t index2 = -1;
        result = request_counter(enclave, &ret, &index2);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: request_counter failed with " << ret << endl;
            // goto exit;
        }

        // get the counter
        uint32_t value = -1;
        result = get_counter(enclave, &ret, &index, 0, 0, nullptr, nullptr, nullptr, &value);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: get_counter failed with " << ret
                 << endl;
            // goto exit;
        }
        cout << "Host: get the " << index << "th counter, value: " << value << endl;

        // get the counter
        result = get_counter(enclave, &ret, &index2, 0, 0, nullptr, nullptr, nullptr, &value);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: get_counter failed with " << ret
                 << endl;
            // goto exit;
        }
        cout << "Host: get the " << index2 << "th counter, value: " << value << endl;

        // get the counter
        result = get_counter(enclave, &ret, &index, 0, 0, nullptr, nullptr, nullptr, &value);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: get_counter failed with " << ret
                 << endl;
            // goto exit;
        }
        cout << "Host: get the " << index << "th counter, value: " << value << endl;
    }

    // random test
    {
        // generate random device rand
        cout << "Host: generate random device rand:" << endl;
        uint32_t rdrandNum = -1;
        result = generate_rdrand(enclave, &ret, &rdrandNum);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: reset_prng failed with " << ret << endl;
            // goto exit;
        }
        cout << "Host: get the rdrandNum: " << rdrandNum << endl;

        // reset prng
        cout << "Host: reset prng:" << endl;
        uint32_t range = 50;
        result = reset_prng(enclave, &ret, &rdrandNum, &range);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: reset_prng failed with " << ret << endl;
            // goto exit;
        }
        cout << "Host: reset prng." << endl;

        // generate random device rand
        cout << "Host: generate random from prng:" << endl;
        uint32_t randNum = -1;
        for (size_t i = 0; i < 10; i++)
        {
            result = generate_rand(enclave, &ret, 0, 0, nullptr, nullptr, nullptr, &randNum);
            cout << "Host: get the randNum: " << randNum << endl;
        }
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: reset_prng failed with " << ret << endl;
            // goto exit;
        }
    }

/*
    // decryptor test
    {
        cout << "Host: generate key" << endl;
        size_t key_size = 2048;
        result = generate_key(enclave, &ret, &key_size);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: generate_key failed with " << ret << endl;
            // goto exit;
        }

        // Simulating binary data
        unsigned char binary_data[] = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe};
        int data_len = sizeof(binary_data) / sizeof(binary_data[0]);

        unsigned char* key_buf;
        size_t key_len = 0;
        result = get_pubkey(enclave, &ret, &key_buf, &key_len);

        BIO *bio = BIO_new_mem_buf(key_buf, -1);
        RSA *pub_key = NULL;
        PEM_read_bio_RSAPublicKey(bio, &pub_key, NULL, NULL);
        
        size_t encrypted_len = RSA_size(pub_key);
        unsigned char* encrypted_data = new unsigned char[encrypted_len]; // Initialize buffer
        std::cout<<"Before RSA_public_encrypt.";
        size_t result_len = RSA_public_encrypt(data_len, binary_data, encrypted_data, 
                                                pub_key, RSA_PKCS1_PADDING);
        std::cout<<"After RSA_public_encrypt.";
        cout << "Host: encrypt data" << endl;
        BIO_free_all(bio);

        cout << "Host: decrypt data" << endl;
        unsigned char * decrypted_data;
        size_t decrypted_len;
        result = decrypt(enclave, &ret, encrypted_data, &decrypted_data, encrypted_len, &decrypted_len);
        if (result != OE_OK)
        {
            ret = 1;
            // goto exit;
        }
        if (ret != 0)
        {
            cerr << "Host: decrypt failed with " << ret << endl;
            // goto exit;
        }

        printHex(binary_data, data_len);
        printHex(decrypted_data, decrypted_len);
        std::cout << "Decrypted data matches original: " << (memcmp(binary_data, decrypted_data, decrypted_len) == 0) << std::endl;

    }
*/

exit:
    cout << "Host: terminate the enclave" << endl;
    cout << "Host: Sample completed successfully." << endl;
    oe_terminate_enclave(enclave);
    return ret;
}


// Command:
// g++ host.cpp /root/code_dev/tee-code/sgx_cpp/build/host/sgx_cpp_u.c  -I/root/code_dev/tee-code/sgx_cpp/build/host -I/opt/openenclave_0_17_0/include/ -L/opt/openenclave_0_17_0/lib/openenclave/host/ -loehost -lcrypto -ldl -lpthread
