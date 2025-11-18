#include "learner/learner.h"

Learner::Learner(const std::string& config_path)
    : config_(resdb::ResDBConfig::FromFile(config_path)),
      manager_(config_) {

    int bs = config_.GetBlockSize();
    std::cout << "Learner block_size = " << bs << std::endl;

    manager_.RegisterMessageHandler(
        [&](const resdb::ResDBMessage& msg){
            // no-op for now
        }
    );
}

void Learner::Run() {
    manager_.Run();
}

