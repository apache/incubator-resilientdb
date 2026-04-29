// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#pragma once

#include <openenclave/enclave.h>
#include <vector>
#include <random>

#include <mbedtls/aes.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <openenclave/enclave.h>
#include <string>


using namespace std;

class ecall_dispatcher
{
  private:
    // counter
    uint32_t counterNum;
    std::vector<uint32_t> counters;
    
    // random generator
    std::random_device rd;  // Obtain a random number from hardware
    uint32_t seed;
    uint32_t range;
    std::mt19937 eng;
    uint32_t randUseCount;

    // decryptor
    unsigned char* pubKeyData;
    unsigned char* privKeyData;
    int pubLen;
    int privLen;

    // checker (Damysus)
    bool checker_initialized_ = false;
    uint32_t checker_node_id_ = 0;
    uint32_t checker_view_ = 0;
    uint32_t checker_phase_ = 0;  // 0=nv_p, 1=prep_p, 2=pcom_p
    uint32_t checker_prepv_ = 0;
    unsigned char checker_preph_[32] = {0};

  public:
    ecall_dispatcher();

    // counter
    int request_counter(uint32_t* index);
    int get_counter(
      uint32_t* index,
      size_t previous_size,
      size_t limit_count,
      uint32_t* counter_value_array,
      size_t * buffer_size_array,
      unsigned char ** previous_attestation,
      uint32_t* counter_value);

    // random generator
    int reset_prng(
      uint32_t* seed, 
      uint32_t* range);
    int generate_rdrand(uint32_t* rdrandNum);
    int generate_rand(size_t previous_size,
                      size_t limit_count,
                      uint32_t* counter_value_array,
                      size_t * buffer_size_array,
                      unsigned char ** previous_attestation,
                      uint32_t* randNum);

    // decryptor
    int generate_key(size_t* key_size);
    int update_key(
      unsigned char* priv_key,
      size_t priv_len,
      unsigned char* pub_key,
      size_t pub_len);
    int get_pubkey(
      unsigned char **key_buf,
      size_t *key_len);
    int encrypt(
      unsigned char* input_buf,
      unsigned char** output_buf,
      size_t input_len,
      size_t* output_len);

    int decrypt(
      unsigned char* input_buf,
      unsigned char** output_buf,
      size_t input_len,
      size_t* output_len);

    // stat
    int get_heap_stats(
      uint64_t* current_heap,
      uint64_t* peak_heap,
      uint64_t* max_heap);

    // checker (Damysus/Achilles)
    int checker_init(uint32_t* node_id);
    int checker_tee_prepare(
      unsigned char* block_hash, size_t hash_len,
      unsigned char* acc_data, size_t acc_len,
      unsigned char** out_cert, size_t* out_cert_len);
    int checker_tee_store(
      unsigned char* block_cert, size_t cert_len,
      unsigned char** out_cert, size_t* out_cert_len);
    int checker_tee_sign(
      unsigned char** out_cert, size_t* out_cert_len);

    // accumulator (Damysus/Achilles)
    int accum_tee_start(
      unsigned char* commitment, size_t commit_len,
      unsigned char** out_acc, size_t* out_acc_len);
    int accum_tee_accum(
      unsigned char* accumulator, size_t acc_len,
      unsigned char* commitment, size_t commit_len,
      unsigned char** out_acc, size_t* out_acc_len);
    int accum_tee_finalize(
      unsigned char* accumulator, size_t acc_len,
      unsigned char** out_acc, size_t* out_acc_len);
};
