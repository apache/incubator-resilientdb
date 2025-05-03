package(default_visibility = ["//platform/consensus/ordering/hs_rl:__subpackages__"])

cc_library(
    name = "graph",
    srcs = ["graph.cpp"],
    hdrs = ["graph.h"],
    deps = [
        "//common:comm",
        "//common/utils",
        "//platform/consensus/ordering/hs_rl/proto:proposal_cc_proto",
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
        "//platform/consensus/ordering/hs_rl/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "themis",
    srcs = ["themis.cpp"],
    hdrs = ["themis.h"],
    deps = [
      ":hs",
        ":proposal_manager",
        ":graph",
        "//common/crypto:signature_verifier",
        "//common:comm",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)

cc_library(
    name = "hs",
    srcs = ["hs.cpp"],
    hdrs = ["hs.h"],
    deps = [
        ":proposal_manager",
        ":graph",
        "//common:comm",
        "//platform/statistic:stats",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
    ],
)
