package(default_visibility = ["//platform/consensus/ordering/pbft_rl:__subpackages__"])

cc_library(
    name = "graph",
    srcs = ["graph.cpp"],
    hdrs = ["graph.h"],
    deps = [
        "//common:comm",
        "//common/utils",
        "//platform/consensus/ordering/pbft_rl/proto:proposal_cc_proto",
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
        "//platform/consensus/ordering/pbft_rl/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "pbft",
    srcs = ["pbft.cpp"],
    hdrs = ["pbft.h"],
    deps = [
        ":proposal_manager",
        ":graph",
        "//common:comm",
        "//platform/statistic:stats",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/consensus/ordering/common/fairness:order_manager",
    ],
)

cc_library(
    name = "themis",
    srcs = ["themis.cpp"],
    hdrs = ["themis.h"],
    deps = [
        "//common/utils",
        ":pbft",
        ":proposal_manager",
        ":graph",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)
