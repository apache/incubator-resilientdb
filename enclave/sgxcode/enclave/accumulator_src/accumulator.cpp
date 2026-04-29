#include "common/dispatcher.h"
#include "common/trace.h"
#include <cstring>

// accum_tee_start: TEEstart from Damysus paper
// Takes a commitment (new-view certificate) and creates initial accumulator
// Accumulator format: [32 bytes hash][4 bytes view][4 bytes just_view][4 bytes count][N*4 bytes node_ids]
int ecall_dispatcher::accum_tee_start(
    unsigned char* commitment, size_t commit_len,
    unsigned char** out_acc, size_t* out_acc_len) {

    if (!commitment || commit_len < 45) return -1;

    // Parse the nv_p commitment: [4 view][1 phase][32 preph][4 prepv][4 node_id]
    unsigned char* p = commitment;
    uint32_t view;
    memcpy(&view, p, 4); p += 4;
    uint8_t phase;
    memcpy(&phase, p, 1); p += 1;
    // preph
    unsigned char preph[32];
    memcpy(preph, p, 32); p += 32;
    // prepv
    uint32_t prepv;
    memcpy(&prepv, p, 4); p += 4;
    // node_id
    uint32_t node_id;
    memcpy(&node_id, p, 4); p += 4;

    // Create accumulator: the initial accumulator records this one node's contribution
    // Format: [32 bytes hash][4 bytes view][4 bytes just_view][4 bytes count=1][4 bytes node_id]
    size_t out_size = 48;
    *out_acc = (unsigned char*)oe_host_malloc(out_size);
    if (!*out_acc) return -1;

    unsigned char* op = *out_acc;
    memcpy(op, preph, 32); op += 32;   // hash of highest prepared block
    memcpy(op, &view, 4); op += 4;     // view
    memcpy(op, &prepv, 4); op += 4;    // just_view (prepared view)
    uint32_t count = 1;
    memcpy(op, &count, 4); op += 4;    // count
    memcpy(op, &node_id, 4); op += 4;  // first node id

    *out_acc_len = out_size;
    return 0;
}

// accum_tee_accum: TEEaccum from Damysus paper
// Takes current accumulator and a new commitment, returns updated accumulator
int ecall_dispatcher::accum_tee_accum(
    unsigned char* accumulator, size_t acc_len,
    unsigned char* commitment, size_t commit_len,
    unsigned char** out_acc, size_t* out_acc_len) {

    if (!accumulator || acc_len < 48) return -1;
    if (!commitment || commit_len < 45) return -1;

    // Parse accumulator: [32 hash][4 view][4 just_view][4 count][N*4 node_ids]
    unsigned char* ap = accumulator;
    unsigned char acc_hash[32];
    memcpy(acc_hash, ap, 32); ap += 32;
    uint32_t acc_view;
    memcpy(&acc_view, ap, 4); ap += 4;
    uint32_t acc_just_view;
    memcpy(&acc_just_view, ap, 4); ap += 4;
    uint32_t acc_count;
    memcpy(&acc_count, ap, 4); ap += 4;
    // node_ids follow

    // Parse commitment: [4 view][1 phase][32 preph][4 prepv][4 node_id]
    unsigned char* cp = commitment;
    uint32_t commit_view;
    memcpy(&commit_view, cp, 4); cp += 4;
    cp += 1; // skip phase
    unsigned char commit_preph[32];
    memcpy(commit_preph, cp, 32); cp += 32;
    uint32_t commit_prepv;
    memcpy(&commit_prepv, cp, 4); cp += 4;
    uint32_t commit_node_id;
    memcpy(&commit_node_id, cp, 4); cp += 4;

    // Check if this node is already in the accumulator
    for (uint32_t i = 0; i < acc_count; i++) {
        uint32_t existing_id;
        memcpy(&existing_id, accumulator + 44 + i * 4, 4);
        if (existing_id == commit_node_id) {
            return -1;  // duplicate node
        }
    }

    // Update: if this commitment has a higher prepared view, update the hash
    unsigned char new_hash[32];
    uint32_t new_just_view = acc_just_view;
    if (commit_prepv > acc_just_view) {
        memcpy(new_hash, commit_preph, 32);
        new_just_view = commit_prepv;
    } else {
        memcpy(new_hash, acc_hash, 32);
    }

    // Build new accumulator with one more node
    uint32_t new_count = acc_count + 1;
    size_t out_size = 44 + new_count * 4;
    *out_acc = (unsigned char*)oe_host_malloc(out_size);
    if (!*out_acc) return -1;

    unsigned char* op = *out_acc;
    memcpy(op, new_hash, 32); op += 32;
    memcpy(op, &acc_view, 4); op += 4;
    memcpy(op, &new_just_view, 4); op += 4;
    memcpy(op, &new_count, 4); op += 4;
    // Copy existing node ids
    memcpy(op, accumulator + 44, acc_count * 4); op += acc_count * 4;
    // Add new node id
    memcpy(op, &commit_node_id, 4); op += 4;

    *out_acc_len = out_size;
    return 0;
}

// accum_tee_finalize: TEEfinalize from Damysus paper
// Finalizes the accumulator (replaces node id list with just the count)
int ecall_dispatcher::accum_tee_finalize(
    unsigned char* accumulator, size_t acc_len,
    unsigned char** out_acc, size_t* out_acc_len) {

    if (!accumulator || acc_len < 44) return -1;

    // Output finalized accumulator: [32 hash][4 view][4 just_view][4 count]
    // The count field already has the number; we just strip the node_id list
    size_t out_size = 44;
    *out_acc = (unsigned char*)oe_host_malloc(out_size);
    if (!*out_acc) return -1;

    memcpy(*out_acc, accumulator, 44);
    *out_acc_len = out_size;
    return 0;
}
