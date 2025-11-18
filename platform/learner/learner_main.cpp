#include "learner/learner.h"

int main(int argc, char** argv) {
    Learner learner("config/learner.config");
    learner.Run();
    return 0;
}

