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

extern "C" int enclave_get_heap_stats(
    uint64_t* current_heap,
    uint64_t* peak_heap,
    uint64_t* max_heap)
{
    return dispatcher.get_heap_stats(current_heap, peak_heap, max_heap);
}

// Checker ECalls
int checker_init(uint32_t* node_id) {
    return dispatcher.checker_init(node_id);
}

int checker_tee_prepare(
    unsigned char* block_hash, size_t hash_len,
    unsigned char* acc_data, size_t acc_len,
    unsigned char** out_cert, size_t* out_cert_len) {
    return dispatcher.checker_tee_prepare(block_hash, hash_len, acc_data, acc_len, out_cert, out_cert_len);
}

int checker_tee_store(
    unsigned char* block_cert, size_t cert_len,
    unsigned char** out_cert, size_t* out_cert_len) {
    return dispatcher.checker_tee_store(block_cert, cert_len, out_cert, out_cert_len);
}

int checker_tee_sign(
    unsigned char** out_cert, size_t* out_cert_len) {
    return dispatcher.checker_tee_sign(out_cert, out_cert_len);
}

// Accumulator ECalls
int accum_tee_start(
    unsigned char* commitment, size_t commit_len,
    unsigned char** out_acc, size_t* out_acc_len) {
    return dispatcher.accum_tee_start(commitment, commit_len, out_acc, out_acc_len);
}

int accum_tee_accum(
    unsigned char* accumulator, size_t acc_len,
    unsigned char* commitment, size_t commit_len,
    unsigned char** out_acc, size_t* out_acc_len) {
    return dispatcher.accum_tee_accum(accumulator, acc_len, commitment, commit_len, out_acc, out_acc_len);
}

int accum_tee_finalize(
    unsigned char* accumulator, size_t acc_len,
    unsigned char** out_acc, size_t* out_acc_len) {
    return dispatcher.accum_tee_finalize(accumulator, acc_len, out_acc, out_acc_len);
}
