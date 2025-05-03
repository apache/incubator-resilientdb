#include "platform/consensus/ordering/common/fairness/order_manager.h"

namespace resdb {
// namespace common {

OrderManager::OrderManager() : fo_count_(0), lo_count_(0), final_ordering_(""), local_ordering_("") {
    fo_file_name_ = "final_ordering.txt";
    lo_file_name_ = "local_ordering.txt";
    interval_ = 100;
}

void OrderManager::AddFinalOrderingRecord(uint64_t proxy_id, uint64_t user_seq, uint64_t scc) {
    std::unique_lock<std::mutex> lk(fo_mutex_);
    final_ordering_ += std::to_string(proxy_id) + " " + std::to_string(user_seq) + " " + std::to_string(fo_count_++) + " " + std::to_string(scc) + "\n";
    if (fo_count_ % interval_ == 0) {
        AppendFO();
    }
}

void OrderManager::AppendFO() {
    std::ofstream file(fo_file_name_, std::ios::app); // Open in append mode
    if (file.is_open()) {
        file << final_ordering_; // Append data with a newline
        final_ordering_.clear();
        file.close();
    } else {
        throw std::runtime_error("Unable to open file for appending.");
    }
}

void OrderManager::AddLocalOrderingRecord(uint64_t proxy_id, uint64_t user_seq) {
    std::unique_lock<std::mutex> lk(lo_mutex_);
    local_ordering_ += std::to_string(proxy_id) + " " + std::to_string(user_seq) + " " + std::to_string(lo_count_++) + "\n";
    if (lo_count_ % interval_ == 0) {
        AppendLO();
    }
}

void OrderManager::AppendLO() {
    std::ofstream file(lo_file_name_, std::ios::app); // Open in append mode
    if (file.is_open()) {
        file << local_ordering_; // Append data with a newline
        local_ordering_.clear();
        file.close();
    } else {
        throw std::runtime_error("Unable to open file for appending.");
    }
}

// void OrderManager::ManipulateLocalOrdering(std::vector<std::unique_ptr<Transaction>>& txns) {
//     std::reverse(txns.begin(), txns.end());
// }

// }
}