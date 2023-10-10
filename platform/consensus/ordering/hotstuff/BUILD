package(default_visibility = ["//visibility:private"])

cc_library(
    name = "message_manager",
    srcs = ["message_manager.cpp"],
    hdrs = ["message_manager.h"],
    deps = [
        "//executor/common:transaction_manager",
        "//platform/common/queue:lock_free_queue",
        "//platform/config:resdb_config",
        "//platform/consensus/execution:system_info",
        "//platform/consensus/execution:transaction_executor",
        "//platform/consensus/ordering/common:message_manager_basic",
        "//platform/consensus/ordering/hotstuff/proto:hotstuff_cc_proto",
        "//platform/networkstrate:server_comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "commitment",
    srcs = ["commitment.cpp"],
    hdrs = ["commitment.h"],
    deps = [
        ":message_manager",
        "//platform/consensus/ordering/common:commitment_basic",
        "//platform/networkstrate:replica_communicator",
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        "//platform/consensus/ordering/hotstuff/proto:hotstuff_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        #"//platform/consensus/ordering/common:performance_generator",
        "//platform/consensus/ordering/hotstuff/proto:hotstuff_cc_proto",
        "//platform/networkstrate:replica_communicator",
        "//platform/networkstrate:server_comm",
    ],
)

cc_library(
    name = "consensus",
    srcs = ["consensus.cpp"],
    hdrs = ["consensus.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":commitment",
        ":message_manager",
        #":performance_manager",
        #":response_manager",
        "//common/crypto:signature_verifier",
        "//platform/consensus/ordering/common:response_manager",
        "//platform/consensus/ordering/common:performance_manager",
        #"//platform/consensus/ordering/common:performance_generator",
        "//platform/networkstrate:consensus_manager",
    ],
)
