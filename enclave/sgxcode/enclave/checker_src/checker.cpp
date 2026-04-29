#include "common/dispatcher.h"
#include "common/trace.h"
#include <cstring>
#include <openssl/sha.h>

// checker_init: initialize checker state
int ecall_dispatcher::checker_init(uint32_t* node_id) {
    checker_node_id_ = *node_id;
    checker_view_ = 0;
    checker_phase_ = 0;  // nv_p
    checker_prepv_ = 0;
    memset(checker_preph_, 0, 32);  // genesis hash = zeros
    checker_initialized_ = true;
    return 0;
}

// checker_tee_prepare: TEEprepare from Damysus paper
// Takes block_hash and returns a serialized commitment (block certificate)
// The commitment is: (block_hash, view, acc_hash, acc_view, prep_p phase)
// This increments the phase from nv_p to prep_p
int ecall_dispatcher::checker_tee_prepare(
    unsigned char* block_hash, size_t hash_len,
    unsigned char* acc_data, size_t acc_len,
    unsigned char** out_cert, size_t* out_cert_len) {

    if (!checker_initialized_) return -1;

    // The commitment output format:
    // [4 bytes view][1 byte phase=1(prep_p)][32 bytes block_hash][32 bytes acc_hash][4 bytes acc_view][4 bytes node_id]
    // Total: 77 bytes
    size_t cert_size = 77;
    *out_cert = (unsigned char*)oe_host_malloc(cert_size);
    if (!*out_cert) return -1;

    unsigned char* p = *out_cert;

    // Write view
    memcpy(p, &checker_view_, 4); p += 4;
    // Write phase (prep_p = 1)
    uint8_t phase = 1;
    memcpy(p, &phase, 1); p += 1;
    // Write block hash
    size_t copy_len = (hash_len < 32) ? hash_len : 32;
    memset(p, 0, 32);
    memcpy(p, block_hash, copy_len); p += 32;
    // Write accumulator hash (first 32 bytes of acc_data, or zeros if none)
    memset(p, 0, 32);
    if (acc_data && acc_len >= 32) {
        memcpy(p, acc_data, 32);
    }
    p += 32;
    // Write accumulator view (next 4 bytes of acc_data, or 0)
    uint32_t acc_view = 0;
    if (acc_data && acc_len >= 36) {
        memcpy(&acc_view, acc_data + 32, 4);
    }
    memcpy(p, &acc_view, 4); p += 4;
    // Write node id
    memcpy(p, &checker_node_id_, 4); p += 4;

    *out_cert_len = cert_size;

    // Advance phase: nv_p(0) -> prep_p(1)
    checker_phase_ = 1;

    return 0;
}

// checker_tee_store: TEEstore from Damysus paper
// Takes a block certificate (from TEEprepare), stores the block info,
// returns a store certificate (pre-commit vote)
int ecall_dispatcher::checker_tee_store(
    unsigned char* block_cert, size_t cert_len,
    unsigned char** out_cert, size_t* out_cert_len) {

    if (!checker_initialized_) return -1;
    if (cert_len < 77) return -1;

    // Parse the input certificate
    unsigned char* p = block_cert;
    uint32_t cert_view;
    memcpy(&cert_view, p, 4); p += 4;
    uint8_t cert_phase;
    memcpy(&cert_phase, p, 1); p += 1;

    // Verify it's a prep_p certificate for current view
    if (cert_phase != 1) return -1;  // must be prep_p
    if (cert_view != checker_view_) return -1;  // must be current view

    // Extract block hash
    unsigned char block_hash[32];
    memcpy(block_hash, p, 32); p += 32;

    // Update prepared block info
    memcpy(checker_preph_, block_hash, 32);
    checker_prepv_ = cert_view;

    // Generate store certificate (pcom_p commitment)
    // Format: [4 bytes view][1 byte phase=2(pcom_p)][32 bytes block_hash][4 bytes node_id]
    size_t out_size = 41;
    *out_cert = (unsigned char*)oe_host_malloc(out_size);
    if (!*out_cert) return -1;

    unsigned char* op = *out_cert;
    memcpy(op, &checker_view_, 4); op += 4;
    uint8_t store_phase = 2;  // pcom_p
    memcpy(op, &store_phase, 1); op += 1;
    memcpy(op, block_hash, 32); op += 32;
    memcpy(op, &checker_node_id_, 4); op += 4;

    *out_cert_len = out_size;

    // Advance phase: prep_p(1) -> pcom_p(2)
    checker_phase_ = 2;

    return 0;
}

// checker_tee_sign: TEEsign from Damysus paper
// Returns a new-view certificate containing the latest prepared block info
int ecall_dispatcher::checker_tee_sign(
    unsigned char** out_cert, size_t* out_cert_len) {

    if (!checker_initialized_) return -1;

    // Generate new-view certificate (nv_p commitment)
    // Format: [4 bytes view][1 byte phase=0(nv_p)][32 bytes preph][4 bytes prepv][4 bytes node_id]
    size_t out_size = 45;
    *out_cert = (unsigned char*)oe_host_malloc(out_size);
    if (!*out_cert) return -1;

    unsigned char* op = *out_cert;
    memcpy(op, &checker_view_, 4); op += 4;
    uint8_t nv_phase = 0;  // nv_p
    memcpy(op, &nv_phase, 1); op += 1;
    memcpy(op, checker_preph_, 32); op += 32;
    memcpy(op, &checker_prepv_, 4); op += 4;
    memcpy(op, &checker_node_id_, 4); op += 4;

    *out_cert_len = out_size;

    // Advance to next view: pcom_p(2) -> next view nv_p(0)
    checker_view_++;
    checker_phase_ = 0;

    return 0;
}
