package(default_visibility = ["//platform/consensus/ordering/fides:__subpackages__"])

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    copts = ["-Iexternal/openenclave"],
    deps = [
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/fides/proto:proposal_cc_proto",
        "//enclave:headers",
    ],
)

cc_library(
    name = "fides",
    srcs = ["fides.cpp"],
    hdrs = ["fides.h"],
    copts = ["-Iexternal/openenclave"],
    deps = [
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
        "//enclave:headers",
        "//platform/proto:resdb_cc_proto",
        "//executor/kv:kv_executor",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//platform/statistic:stats",
    ],
)
