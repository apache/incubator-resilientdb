package(default_visibility = ["//platform/consensus/ordering/cassandra_cft:__subpackages__"])

cc_library(
    name = "proposal_state",
    hdrs = ["proposal_state.h"],
)

cc_library(
    name = "proposal_manager",
    srcs = ["proposal_manager.cpp"],
    hdrs = ["proposal_manager.h"],
    deps = [
        ":proposal_graph",
        "//common:comm",
        "//platform/statistic:stats",
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/cassandra_cft/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "ranking",
    srcs = ["ranking.cpp"],
    hdrs = ["ranking.h"],
    deps = [
        "//common:comm",
    ],
)

cc_library(
    name = "proposal_graph",
    srcs = ["proposal_graph.cpp"],
    hdrs = ["proposal_graph.h"],
    deps = [
        ":ranking",
        ":proposal_state",
        "//platform/statistic:stats",
        "//common:comm",
        "//common/utils",
        "//platform/consensus/ordering/cassandra_cft/proto:proposal_cc_proto",
    ],
)

cc_library(
    name = "cassandra",
    srcs = ["cassandra.cpp"],
    hdrs = ["cassandra.h"],
    deps = [
        ":proposal_graph",
        ":proposal_manager",
        "//platform/statistic:stats",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/common/queue:lock_free_queue",
    ],
)

cc_test(
    name = "proposal_graph_test",
    srcs = ["proposal_graph_test.cpp"],
    deps = [
        ":proposal_graph",
        "//common/test:test_main",
    ],
)

cc_test(
    name = "cassandra_cft_test",
    srcs = ["cassandra_cft_test.cpp"],
    deps = [
        ":cassandra",
        "//common/test:test_main",
    ],
)
