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

};
