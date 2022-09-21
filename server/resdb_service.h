#pragma once

#include <atomic>
#include <memory>

#include "common/data_comm/data_comm.h"
#include "server/server_comm.h"

namespace resdb {

class ResDBService {
 public:
  ResDBService() : is_running_(false) {}
  virtual ~ResDBService() = default;

  virtual int Process(std::unique_ptr<Context> context,
                      std::unique_ptr<DataInfo> request_info);
  virtual bool IsRunning() const;
  virtual bool IsReady() const { return false; }
  virtual void SetRunning(bool is_running);
  virtual void Start();
  virtual void Stop();

 private:
  std::atomic<bool> is_running_;
};

}  // namespace resdb
