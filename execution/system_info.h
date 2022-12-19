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

#include <memory>

#include "config/resdb_config.h"
#include "proto/resdb.pb.h"

namespace resdb {

// SystemInfo managers the cluster information which
// has been agreed on, like the primary, the replicas,etc..
class SystemInfo {
 public:
  SystemInfo(const ResDBConfig& config);
  virtual ~SystemInfo() = default;

  std::vector<ReplicaInfo> GetReplicas() const;
  void SetReplicas(const std::vector<ReplicaInfo>& replicas);
  void AddReplica(const ReplicaInfo& replica);

  void ProcessRequest(const SystemInfoRequest& request);

  uint32_t GetPrimaryId() const;
  void SetPrimary(uint32_t id);

  uint64_t GetCurrentView() const;
  void SetCurrentView(uint64_t);

 private:
  std::vector<ReplicaInfo> replicas_;
  std::atomic<uint32_t> primary_id_;
  std::atomic<uint64_t> view_;
};
}  // namespace resdb
