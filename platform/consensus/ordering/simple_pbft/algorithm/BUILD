package(default_visibility = ["//platform/consensus/ordering/simple_pbft:__subpackages__"])

cc_library(
    name = "pbft",
    srcs = ["pbft.cpp"],
    hdrs = ["pbft.h"],
    deps = [
        "//platform/statistic:stats",
        "//common:comm",
        "//platform/consensus/ordering/simple_pbft/proto:proposal_cc_proto",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/config:resdb_config",
    ],
)
