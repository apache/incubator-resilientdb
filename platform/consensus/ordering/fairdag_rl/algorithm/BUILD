package(default_visibility = ["//platform/consensus/ordering/fairdag_rl:__subpackages__"])

cc_library(
    name = "graph",
    srcs = ["graph.cpp"],
    hdrs = ["graph.h"],
    deps = [
        "//common:comm",
        "//common/utils",
        "//platform/consensus/ordering/fairdag_rl/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/fairdag_rl/proto:proposal_cc_proto",
    ],
)

cc_test (
    name = "graph_test",
    srcs = ["graph_test.cpp"],
    deps = [
        ":graph",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "fairdag",
    srcs = ["fairdag.cpp"],
    hdrs = ["fairdag.h"],
    deps = [
      ":tusk",
        ":proposal_manager",
        ":graph",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)

cc_library(
    name = "tusk",
    srcs = ["tusk.cpp"],
    hdrs = ["tusk.h"],
    deps = [
        ":proposal_manager",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
    ],
)

