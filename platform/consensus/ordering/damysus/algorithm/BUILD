package(default_visibility = ["//platform/consensus/ordering/damysus:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/damysus/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "damysus",
    srcs = ["damysus.cpp"],
    hdrs = ["damysus.h"],
    copts = ["-Iexternal/openenclave"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
        "//enclave:headers",
        "//platform/config:resdb_config",
    ],
)
