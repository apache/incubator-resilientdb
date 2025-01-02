#include "common/dispatcher.h"
#include "common/trace.h"

int ecall_dispatcher::reset_prng(uint32_t* seed, uint32_t* range) {
    int ret = 0;
    // TRACE_ENCLAVE("ecall_dispatcher::set_seed");

    this->seed = *seed;
    this->range = *range;
    eng.seed(*seed);
    this->randUseCount = 0;

exit:
    return ret;
}

int ecall_dispatcher::generate_rdrand(uint32_t* rdrandNum) {
    int ret = 0;
    // TRACE_ENCLAVE("ecall_dispatcher::generate_rdrand");
    
    *rdrandNum = rd();

exit:
    return ret;  
}

int ecall_dispatcher::generate_rand(size_t previous_size,
                                    size_t limit_count,
                                    uint32_t* counter_value_array,
                                    size_t * buffer_size_array,
                                    unsigned char ** previous_attestation,
                                    uint32_t* randNum) {
    int ret = 0;
    // TRACE_ENCLAVE("ecall_dispatcher::generate_rdrand");
    
    // TRACE_ENCLAVE("counter_value:%d, randUseCount:%d, generate_rand",counter_value_array[0],randUseCount);
    
    if (randUseCount != 0 && previous_size < limit_count) {
        TRACE_ENCLAVE("%d,%d: Insufficient previous cert number",previous_size,limit_count);
        goto exit;
    }

    // Check previous counter info and attestation here
    for (size_t i = 0; i < previous_size; i++)
    {
        if (!std::strncmp((const char*)previous_attestation[i], "Fake Attestation", buffer_size_array[i]) == 0) {
            TRACE_ENCLAVE("Could not verify the attestation");
            goto exit;
        }
        if (counter_value_array[i]/3 + 1 != randUseCount) {
            TRACE_ENCLAVE("counter_value:%d, randUseCount:%d, Counter value invalid",counter_value_array[i],randUseCount);
            goto exit;
        }
        // TRACE_ENCLAVE("previous_size: %d, counter: %d, attestation: %.*s", previous_size, 
        //             counter_value_array[i], buffer_size_array[i], previous_attestation[i]);
    }

    *randNum = eng() % range;
    randUseCount++;

exit:
    return ret;  
}
