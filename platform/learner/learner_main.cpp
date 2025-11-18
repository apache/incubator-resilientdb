#include "platform/learner/learner.h"

#include <exception>
#include <iostream>
#include <string>

std::string ResolveConfigPath(int argc, char** argv) {
    std::string config_path = "platform/learner/learner.config";
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-config" && i + 1 < argc) {
        config_path = argv[i + 1];
        break;
        }
    }
    return config_path;
}

int main(int argc, char** argv) {
    try {
        Learner learner(ResolveConfigPath(argc, argv));
        learner.Run();
    } catch (const std::exception& e) {
        std::cerr << "Failed to start learner: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
