#pragma once

#include <string>
#include <deque>
#include <map>
#include <queue>
#include <mutex>
#include <fstream>
#include <memory>
#include <algorithm>
#include "platform/proto/resdb.pb.h"

namespace resdb {
// namespace common {

class OrderManager{
    public:
    OrderManager();
    void AddFinalOrderingRecord(uint64_t proxy_id, uint64_t user_seq, uint64_t scc);
    void AddLocalOrderingRecord(uint64_t proxy_id, uint64_t user_seq);

    private:
    void AppendFO();
    void AppendLO();
    std::mutex fo_mutex_, lo_mutex_;
    std::string final_ordering_, local_ordering_;
    uint64_t fo_count_, lo_count_;
    uint64_t interval_;
    std::string fo_file_name_, lo_file_name_;
};

// }
}