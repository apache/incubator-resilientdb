#include "common/dispatcher.h"
#include "common/trace.h"
#include <cstring>

int ecall_dispatcher::request_counter(uint32_t* index) {
    int ret = 0;
    // TRACE_ENCLAVE("ecall_dispatcher::request_counter");

    counters.push_back(0);
    *index = counterNum;
    counterNum++;

exit:
    return ret;
}

int ecall_dispatcher::get_counter(
    uint32_t* index,
    size_t previous_size,
    size_t limit_count,
    uint32_t* counter_value_array,
    size_t * buffer_size_array,
    unsigned char ** previous_attestation,
    uint32_t* counter_value) {
    
    int ret = 0;
    
    if (counters[*index]!=0 && previous_size < limit_count) {
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
        if (counter_value_array[i] != counters[*index] - 1) {
            TRACE_ENCLAVE("Counter value invalid");
            goto exit;
        }
        // TRACE_ENCLAVE("previous_size: %d, counter: %d, attestation: %.*s", previous_size, 
        //             counter_value_array[i], buffer_size_array[i], previous_attestation[i]);
    }
    // */

    if (*index < 0 || *index >= counterNum) {
        // TRACE_ENCLAVE("Counter index out of range");
        goto exit;
    }

    *counter_value = counters[*index];
    counters[*index]++;

exit:
    return ret;
}
