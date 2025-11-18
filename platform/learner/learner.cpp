#include "platform/learner/learner.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "platform/common/network/socket.h"
#include "platform/common/network/tcp_socket.h"
#include "platform/proto/resdb.pb.h"

namespace {

std::string DefaultConfigPath() { return "platform/learner/learner.config"; }

LearnerConfig ParseLearnerConfig(const std::string& config_path) {
    std::ifstream config_stream(config_path);
    if (!config_stream.is_open()) {
        throw std::runtime_error("Unable to open learner config: " + config_path);
    }

    nlohmann::json config_json =
        nlohmann::json::parse(config_stream, nullptr, true, true);
    LearnerConfig config;
    config.ip = config_json.value("ip", config.ip);
    config.port = config_json.value("port", 0);
    config.block_size = config_json.value("block_size", 0);

    if (config.port <= 0) {
        throw std::runtime_error("Learner config must include a valid port");
    }
    if (config.block_size <= 0) {
        throw std::runtime_error("Learner config must include a positive block_size");
    }
    return config;
}

}  // namespace

Learner::Learner(const std::string& config_path)
    : config_(LoadConfig(config_path)) {}

LearnerConfig Learner::LoadConfig(const std::string& config_path) {
    const std::string path = config_path.empty() ? DefaultConfigPath() : config_path;
    return ParseLearnerConfig(path);
}

void Learner::Run() {
    resdb::TcpSocket server;
        if (server.Listen(config_.ip, config_.port) != 0) {
            throw std::runtime_error("Learner failed to bind to " + config_.ip + ":" +
                                    std::to_string(config_.port));
    }

    is_running_.store(true);
    metrics_thread_ = std::thread(&Learner::MetricsLoop, this);

    while (is_running_.load()) {
        auto client = server.Accept();
        if (!client) {
        continue;
        }
        std::thread(&Learner::HandleClient, this, std::move(client)).detach();
    }
}

void Learner::HandleClient(std::unique_ptr<resdb::Socket> socket) const {
    std::cout << "[Learner] replica connected" << std::endl;
    while (true) {
        void* buffer = nullptr;
        size_t len = 0;
        int ret = socket->Recv(&buffer, &len);
        if (ret <= 0) {
        if (buffer != nullptr) {
            free(buffer);
        }
        break;
        }

        std::string payload(static_cast<char*>(buffer), len);
        free(buffer);
        ProcessBroadcast(payload);
    }
    std::cout << "[Learner] replica disconnected" << std::endl;
}

void Learner::ProcessBroadcast(const std::string& payload) const {
    resdb::ResDBMessage envelope;
    if (!envelope.ParseFromString(payload)) {
        ++total_messages_;
        total_bytes_.fetch_add(payload.size());
        last_type_.store(-1);
        last_seq_.store(0);
        last_sender_.store(-1);
        last_payload_bytes_.store(payload.size());
        return;
    }

    resdb::Request request;
    if (request.ParseFromString(envelope.data())) {
        ++total_messages_;
        total_bytes_.fetch_add(payload.size());
        last_type_.store(request.type());
        last_seq_.store(request.seq());
        last_sender_.store(request.sender_id());
        last_payload_bytes_.store(request.data().size());
        return;
    }

    ++total_messages_;
    total_bytes_.fetch_add(envelope.data().size());
    last_type_.store(-1);
    last_seq_.store(0);
    last_sender_.store(-1);
    last_payload_bytes_.store(envelope.data().size());
}

void Learner::MetricsLoop() const {
    while (is_running_.load()) {
        PrintMetrics();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Learner::PrintMetrics() const {
    std::cout << "[Learner] broadcasts=" << total_messages_.load()
        << " bytes=" << total_bytes_.load()
        << " last_type=" << last_type_.load()
        << " last_seq=" << last_seq_.load()
        << " sender=" << last_sender_.load()
        << " payload=" << last_payload_bytes_.load() << "B"
        << " (listening on " << config_.ip << ":" << config_.port
        << ", block_size=" << config_.block_size << ")" << std::endl;
}
