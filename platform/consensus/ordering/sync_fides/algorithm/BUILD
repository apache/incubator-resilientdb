package(default_visibility = ["//platform/consensus/ordering/sync_fides:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    copts = ["-Iexternal/openenclave"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/sync_fides/proto:proposal_cc_proto",
        "//enclave:headers",
        "//platform/config:resdb_config"
    ],
)

cc_library(
    name = "sync_fides",
    srcs = ["sync_fides.cpp"],
    hdrs = ["sync_fides.h"],
    copts = ["-Iexternal/openenclave"],
    deps = [
        ":utility",
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
        "//enclave:headers",
        "//platform/proto:resdb_cc_proto",
        "//executor/kv:kv_executor",
        "//platform/config:resdb_config"
    ],
)

cc_library(
    name = "utility",
    srcs = ["utility.cpp"],
    hdrs = ["utility.h"],
    visibility = ["//platform/consensus/ordering/sync_fides:__subpackages__"],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/statistic:stats",
    ],
)
