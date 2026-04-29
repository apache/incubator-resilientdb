package(default_visibility = ["//platform/consensus/ordering/bullshark:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/bullshark/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "bullshark",
    srcs = ["bullshark.cpp"],
    hdrs = ["bullshark.h"],
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
