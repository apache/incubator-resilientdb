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

#pragma once

#include <chrono>
#include <cstdint>

// Unmangled trace hooks intended for user-space uprobes (e.g., bpftrace).
//
// Design goals:
// - stable, easy-to-type symbol names (no C++ mangling)
// - minimal overhead
// - arguments are plain integers (no std::string, no protobuf types)
//
// Convention:
//   seq: PBFT sequence number
//   sender_id: sender replica id when applicable, else 0
//   self_id: local replica id
//   proxy_id: client proxy id when available, else 0

extern "C" {

// Existing request-level hook at ConsensusCommit entry.
//   arg0=req_ptr, arg1=seq, arg2=type, arg3=sender_id, arg4=proxy_id
void resdb_trace_consensus_commit(uint64_t req_ptr, uint64_t seq, uint32_t type,
                                 uint32_t sender_id, uint64_t proxy_id);

// PBFT phase hooks (mirrors the phases Stats emits).
// NOTE: bpftrace user-space uprobes on x86_64 only expose arg0..arg5.
// To keep hooks universally traceable, we pack fields into a single 64-bit
// meta value:
//   meta[0..31]   = type (uint32)
//   meta[32..47]  = sender_id (uint16)
//   meta[48..63]  = self_id   (uint16)
// epoch_ns is nanoseconds since Unix epoch (system_clock / CLOCK_REALTIME).
void resdb_trace_pbft_request(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                             uint64_t proxy_id, int64_t epoch_ns);
void resdb_trace_pbft_pre_prepare(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                 uint64_t proxy_id, int64_t epoch_ns);

void resdb_trace_pbft_prepare_recv(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                  uint64_t proxy_id, int64_t epoch_ns);
void resdb_trace_pbft_prepare_state(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                   uint64_t proxy_id, int64_t epoch_ns);

void resdb_trace_pbft_commit_recv(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                 uint64_t proxy_id, int64_t epoch_ns);
void resdb_trace_pbft_commit_state(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                  uint64_t proxy_id, int64_t epoch_ns);

void resdb_trace_pbft_execute_start(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                   uint64_t proxy_id, int64_t epoch_ns);
void resdb_trace_pbft_execute_end(uint64_t req_ptr, uint64_t seq, uint64_t meta,
                                 uint64_t proxy_id, int64_t epoch_ns);

}  // extern "C"

// Helpers for building/decoding the packed meta field used by PBFT phase hooks.
// Layout:
//   meta[0..31]   = type (uint32)
//   meta[32..47]  = sender_id (uint16)
//   meta[48..63]  = self_id   (uint16)
inline uint64_t ResdbTracePackMeta(uint32_t type, uint32_t sender_id,
                                                                    uint32_t self_id) {
    return (static_cast<uint64_t>(type) & 0xffffffffULL) |
                 ((static_cast<uint64_t>(sender_id) & 0xffffULL) << 32) |
                 ((static_cast<uint64_t>(self_id) & 0xffffULL) << 48);
}

inline uint32_t ResdbTraceMetaType(uint64_t meta) {
    return static_cast<uint32_t>(meta & 0xffffffffULL);
}

inline uint32_t ResdbTraceMetaSender(uint64_t meta) {
    return static_cast<uint32_t>((meta >> 32) & 0xffffULL);
}

inline uint32_t ResdbTraceMetaSelf(uint64_t meta) {
    return static_cast<uint32_t>((meta >> 48) & 0xffffULL);
}

// Epoch nanoseconds (system_clock) for compatibility with Stats.cpp.
inline int64_t ResdbTraceEpochNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
            .count();
}
