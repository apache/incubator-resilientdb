package(default_visibility = ["//visibility:private"])

cc_library(
    name = "lock_free_collector_pool",
    srcs = ["lock_free_collector_pool.cpp"],
    hdrs = ["lock_free_collector_pool.h"],
    deps = [
        ":transaction_collector",
    ],
)

cc_test(
    name = "lock_free_collector_pool_test",
    srcs = ["lock_free_collector_pool_test.cpp"],
    deps = [
        ":lock_free_collector_pool",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "response_manager",
    srcs = ["response_manager.cpp"],
    hdrs = ["response_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        ":transaction_utils",
        "//server:resdb_replica_client",
    ],
)

cc_test(
    name = "response_manager_test",
    srcs = ["response_manager_test.cpp"],
    deps = [
        ":response_manager",
        "//client:mock_resdb_client",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//server:mock_resdb_replica_client",
    ],
)

cc_library(
    name = "performance_manager",
    srcs = ["performance_manager.cpp"],
    hdrs = ["performance_manager.h"],
    deps = [
        ":lock_free_collector_pool",
        ":transaction_utils",
        "//server:resdb_replica_client",
    ],
)

cc_library(
    name = "transaction_manager",
    srcs = ["transaction_manager.cpp"],
    hdrs = ["transaction_manager.h"],
    deps = [
        ":checkpoint_manager",
        ":lock_free_collector_pool",
        ":transaction_collector",
        ":transaction_utils",
        "//config:resdb_config",
        "//database:txn_memory_db",
        "//execution:transaction_executor_impl",
        "//proto:resdb_cc_proto",
        "//server:server_comm",
    ],
)

cc_library(
    name = "transaction_utils",
    srcs = ["transaction_utils.cpp"],
    hdrs = ["transaction_utils.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "commitment",
    srcs = ["commitment.cpp"],
    hdrs = ["commitment.h"],
    deps = [
        ":response_manager",
        ":transaction_manager",
        "//common/queue:batch_queue",
        "//common/utils",
        "//config:resdb_config",
        "//proto:resdb_cc_proto",
        "//server:resdb_replica_client",
        "//statistic:stats",
    ],
)

cc_test(
    name = "commitment_test",
    srcs = ["commitment_test.cpp"],
    deps = [
        ":commitment",
        "//client:mock_resdb_client",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:mock_signature_verifier",
        "//server:mock_resdb_replica_client",
    ],
)

cc_library(
    name = "checkpoint_manager",
    srcs = ["checkpoint_manager.cpp"],
    hdrs = ["checkpoint_manager.h"],
    deps = [
        ":transaction_utils",
        "//config:resdb_config",
        "//crypto:signature_verifier",
        "//database:txn_memory_db",
        "//proto:checkpoint_info_cc_proto",
        "//server:resdb_replica_client",
        "//server:server_comm",
    ],
)

cc_test(
    name = "checkpoint_manager_test",
    srcs = ["checkpoint_manager_test.cpp"],
    tags = ["manual"],
    deps = [
        ":checkpoint_manager",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:mock_signature_verifier",
        "//server:mock_resdb_replica_client",
        "//statistic:stats",
    ],
)

cc_library(
    name = "viewchange_manager",
    srcs = ["viewchange_manager.cpp"],
    hdrs = ["viewchange_manager.h"],
    deps = [
        ":checkpoint_manager",
        ":transaction_manager",
        ":transaction_utils",
        "//config:resdb_config",
        "//execution:system_info",
        "//proto:viewchange_message_cc_proto",
        "//server:resdb_replica_client",
    ],
)

cc_test(
    name = "viewchange_manager_test",
    srcs = ["viewchange_manager_test.cpp"],
    deps = [
        ":viewchange_manager",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:mock_signature_verifier",
        "//execution:system_info",
        "//server:mock_resdb_replica_client",
    ],
)

cc_library(
    name = "query",
    srcs = ["query.cpp"],
    hdrs = ["query.h"],
    deps = [
        ":transaction_manager",
        "//config:resdb_config",
        "//execution:custom_query",
        "//proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "query_test",
    srcs = ["query_test.cpp"],
    deps = [
        ":commitment",
        ":query",
        "//client:mock_resdb_client",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:mock_signature_verifier",
        "//execution:mock_custom_query",
        "//server:mock_resdb_replica_client",
    ],
)

cc_library(
    name = "transaction_collector",
    srcs = ["transaction_collector.cpp"],
    hdrs = ["transaction_collector.h"],
    deps = [
        "//execution:transaction_executor",
        "//proto:resdb_cc_proto",
        "//server:server_comm",
        "//statistic:stats",
    ],
)

cc_test(
    name = "transaction_collector_test",
    srcs = ["transaction_collector_test.cpp"],
    deps = [
        ":transaction_collector",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "consensus_service_pbft",
    srcs = ["consensus_service_pbft.cpp"],
    hdrs = ["consensus_service_pbft.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":checkpoint_manager",
        ":commitment",
        ":performance_manager",
        ":query",
        ":transaction_manager",
        ":viewchange_manager",
        "//crypto:signature_verifier",
        "//server:consensus_service",
    ],
)
