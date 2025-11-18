#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace resdb {
class Socket;
class Request;
}

struct LearnerConfig {
    std::string ip = "127.0.0.1";
    int port = 0;
    int block_size = 0;
};

class Learner {
    public:
        explicit Learner(const std::string& config_path);
        void Run();

private:
    static LearnerConfig LoadConfig(const std::string& config_path);
    void HandleClient(std::unique_ptr<resdb::Socket> socket) const;
    void ProcessBroadcast(const std::string& payload) const;
    void MetricsLoop() const;
    void PrintMetrics() const;

private:
    LearnerConfig config_;
    std::atomic<bool> is_running_{false};
    mutable std::atomic<uint64_t> total_messages_{0};
    mutable std::atomic<uint64_t> total_bytes_{0};
    mutable std::atomic<int> last_type_{-1};
    mutable std::atomic<int64_t> last_seq_{0};
    mutable std::atomic<int> last_sender_{-1};
    mutable std::atomic<size_t> last_payload_bytes_{0};
    mutable std::thread metrics_thread_;
};
