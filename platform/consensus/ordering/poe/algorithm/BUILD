package(default_visibility = ["//platform/consensus/ordering/poe:__subpackages__"])

cc_library(
    name = "poe",
    srcs = ["poe.cpp"],
    hdrs = ["poe.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/consensus/ordering/poe/proto:proposal_cc_proto",
        "//platform/statistic:stats",
    ],
)
