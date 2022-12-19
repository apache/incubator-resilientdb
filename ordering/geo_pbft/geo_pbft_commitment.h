/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "config/resdb_config.h"
#include "execution/geo_global_executor.h"
#include "execution/system_info.h"
#include "hash_set.h"
#include "proto/resdb.pb.h"
#include "server/resdb_replica_client.h"
#include "server/server_comm.h"

namespace resdb {

class GeoPBFTCommitment {
 public:
  GeoPBFTCommitment(std::unique_ptr<GeoGlobalExecutor> global_executor,
                    const ResDBConfig& config,
                    std::unique_ptr<SystemInfo> system_info_,
                    ResDBReplicaClient* replica_client_,
                    SignatureVerifier* verifier);

  ~GeoPBFTCommitment();

  int GeoProcessCcm(std::unique_ptr<Context> context,
                    std::unique_ptr<Request> request);

 private:
  bool VerifyCerts(const BatchClientRequest& request,
                   const std::string& raw_data);

  bool AddNewReq(uint64_t seq, uint32_t sender_region);
  void UpdateSeq(uint64_t seq);

  int PostProcessExecutedMsg();

 private:
  std::unique_ptr<GeoGlobalExecutor> global_executor_;
  std::atomic<bool> stop_;
  std::set<uint32_t> checklist_[1 << 20];
  ResDBConfig config_;
  std::unique_ptr<SystemInfo> system_info_ = nullptr;
  ResDBReplicaClient* replica_client_;
  SignatureVerifier* verifier_;
  Stats* global_stats_;
  std::mutex mutex_;
  std::thread executed_thread_;
  uint64_t min_seq_ = 0;
};

}  // namespace resdb
