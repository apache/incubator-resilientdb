#include "server/resdb_service.h"

#include <glog/logging.h>

namespace resdb {

int ResDBService::Process(std::unique_ptr<Context> context,
                          std::unique_ptr<DataInfo> request_info) {
  return 0;
}

bool ResDBService::IsRunning() const { return is_running_; }

void ResDBService::SetRunning(bool is_running) { is_running_ = is_running; }

void ResDBService::Start() { SetRunning(true); }
void ResDBService::Stop() { SetRunning(false); }

}  // namespace resdb
