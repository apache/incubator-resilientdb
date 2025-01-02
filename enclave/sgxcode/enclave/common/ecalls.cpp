// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <openenclave/enclave.h>

#include "dispatcher.h"
#include "sgx_cpp_t.h"

// Declare a static dispatcher object for enabling for better organization
// of enclave-wise global variables
static ecall_dispatcher dispatcher;

int request_counter(
    uint32_t* index)
{
    return dispatcher.request_counter(index);
}

int get_counter(
    uint32_t* index,
    size_t previous_size,
    size_t limit_count,
    uint32_t* counter_value_array,
    size_t * buffer_size_array,
    unsigned char ** previous_attestation,
    uint32_t* counter_value)
{
    return dispatcher.get_counter(index,previous_size,limit_count,counter_value_array,buffer_size_array,previous_attestation,counter_value);
}

int reset_prng(
    uint32_t* seed, 
    uint32_t* range)
{
    return dispatcher.reset_prng(seed, range);
}

int generate_rdrand(uint32_t* rdrandNum)
{
    return dispatcher.generate_rdrand(rdrandNum);
}

int generate_rand(size_t previous_size,
                  size_t limit_count,
                  uint32_t* counter_value_array,
                  size_t * buffer_size_array,
                  unsigned char ** previous_attestation,
                  uint32_t* randNum)
{
    return dispatcher.generate_rand(previous_size, limit_count, counter_value_array, buffer_size_array, 
                                    previous_attestation, randNum);
}

int generate_key(
    size_t *key_size)
{
    return dispatcher.generate_key(key_size);
}

int update_key(
    unsigned char* priv_key,
    size_t priv_len,
    unsigned char* pub_key,
    size_t pub_len)
{
    return dispatcher.update_key(priv_key, priv_len, pub_key, pub_len);
}

int get_pubkey(
    unsigned char **key_buf,
    size_t *key_len)
{
    return dispatcher.get_pubkey(key_buf, key_len);
}

int encrypt(
    unsigned char* input_buf,
    unsigned char** output_buf,
    size_t input_len,
    size_t* output_len)
{
    return dispatcher.encrypt(input_buf, output_buf, input_len, output_len);
}

int decrypt(
    unsigned char* input_buf,
    unsigned char** output_buf,
    size_t input_len,
    size_t* output_len)
{
    return dispatcher.decrypt(input_buf, output_buf, input_len, output_len);
}

