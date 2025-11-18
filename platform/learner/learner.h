#pragma once
#include "server/connection_manager.h"
#include "common/config/resdb_config.h"

class Learner {
public:
    explicit Learner(const std::string& config_path);
    void Run();

private:
    resdb::ResDBConfig config_;
    resdb::ConnectionManager manager_;
};

