package(default_visibility = ["//platform/consensus/ordering/mysticeti:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/mysticeti/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "mysticeti",
    srcs = ["mysticeti.cpp"],
    hdrs = ["mysticeti.h"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
        "//platform/config:resdb_config",
    ],
)
