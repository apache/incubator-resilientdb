package(default_visibility = ["//platform/consensus/ordering/simple_pbft:__subpackages__"])

cc_library(
    name = "pbft",
    srcs = ["pbft.cpp"],
    hdrs = ["pbft.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/consensus/ordering/simple_pbft/proto:proposal_cc_proto",
        "//platform/statistic:stats",
    ],
)
