/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/statistic/trace_hooks.h"

// Keep the hook functions and their arguments from being optimized away.
//
// These functions intentionally do not do any work besides providing a stable
// symbol that uprobes can attach to and a well-defined argument ABI.

extern "C" {

__attribute__((noinline)) void resdb_trace_consensus_commit(
    uint64_t req_ptr, uint64_t seq, uint32_t type, uint32_t sender_id,
    uint64_t proxy_id) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(type), "r"(sender_id),
               "r"(proxy_id)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_request(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_pre_prepare(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_prepare_recv(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_prepare_state(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_commit_recv(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_commit_state(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_execute_start(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

__attribute__((noinline)) void resdb_trace_pbft_execute_end(
    uint64_t req_ptr, uint64_t seq, uint64_t meta, uint64_t proxy_id,
    int64_t epoch_ns) {
  asm volatile("" : : "r"(req_ptr), "r"(seq), "r"(meta), "r"(proxy_id),
               "r"(epoch_ns)
               : "memory");
}

}  // extern "C"
