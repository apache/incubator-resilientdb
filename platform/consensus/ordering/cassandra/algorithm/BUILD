package(default_visibility = ["//platform/consensus/ordering/cassandra:__subpackages__"])

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
        "//common/crypto:signature_verifier",
        "//common/utils",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
        "//platform/statistic:stats",
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
        ":proposal_state",
        ":ranking",
        "//common:comm",
        "//common/utils",
        "//platform/consensus/ordering/cassandra/proto:proposal_cc_proto",
        "//platform/statistic:stats",
    ],
)

cc_library(
    name = "cassandra",
    srcs = ["cassandra.cpp"],
    hdrs = ["cassandra.h"],
    deps = [
        ":proposal_graph",
        ":proposal_manager",
        "//common:comm",
        "//common/crypto:signature_verifier",
        "//platform/common/queue:lock_free_queue",
        "//platform/consensus/ordering/common/algorithm:protocol_base",
        "//platform/statistic:stats",
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
    name = "cassandra_test",
    srcs = ["cassandra_test.cpp"],
    deps = [
        ":cassandra",
        "//common/test:test_main",
    ],
)
