package(default_visibility = ["//ordering:__subpackages__"])

cc_library(
    name = "transaction_executor_impl",
    srcs = ["transaction_executor_impl.cpp"],
    hdrs = ["transaction_executor_impl.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//common:comm",
        "//proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "system_info",
    srcs = ["system_info.cpp"],
    hdrs = ["system_info.h"],
    deps = [
        "//config:resdb_config",
        "//proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "system_info_test",
    srcs = ["system_info_test.cpp"],
    deps = [
        ":system_info",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "mock_transaction_executor_impl",
    hdrs = ["mock_transaction_executor_impl.h"],
    deps = [
        ":transaction_executor",
    ],
)

cc_library(
    name = "transaction_executor",
    srcs = ["transaction_executor.cpp"],
    hdrs = ["transaction_executor.h"],
    deps = [
        ":system_info",
        ":transaction_executor_impl",
        "//common:comm",
        "//common/queue:lock_free_queue",
        "//config:resdb_config",
        "//proto:resdb_cc_proto",
        "//statistic:stats",
    ],
)

cc_test(
    name = "transaction_executor_test",
    srcs = ["transaction_executor_test.cpp"],
    deps = [
        ":mock_transaction_executor_impl",
        ":system_info",
        ":transaction_executor",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "geo_transaction_executor",
    srcs = ["geo_transaction_executor.cpp"],
    hdrs = ["geo_transaction_executor.h"],
    deps = [
        ":system_info",
        ":transaction_executor",
        "//ordering/pbft:transaction_utils",
        "//server:resdb_replica_client",
    ],
)

cc_library(
    name = "geo_global_executor",
    srcs = ["geo_global_executor.cpp"],
    hdrs = ["geo_global_executor.h"],
    deps = [
        ":transaction_executor_impl",
        "//application/geo_pbft:geo_pbft_executor_impl",
        "//common/queue:lock_free_queue",
        "//config:resdb_config",
        "//statistic:stats",
    ],
)

cc_test(
    name = "geo_global_executor_test",
    srcs = ["geo_global_executor_test.cpp"],
    deps = [
        ":geo_global_executor",
        ":mock_transaction_executor_impl",
        "//common/test:test_main",
        "//config:resdb_config_utils",
    ],
)

cc_library(
    name = "mock_geo_global_executor",
    hdrs = ["mock_geo_global_executor.h"],
    deps = [
        ":geo_global_executor",
    ],
)

cc_test(
    name = "geo_transaction_executor_test",
    srcs = ["geo_transaction_executor_test.cpp"],
    deps = [
        ":geo_transaction_executor",
        ":mock_transaction_executor_impl",
        "//common/test",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//server:mock_resdb_replica_client",
    ],
)

cc_library(
    name = "custom_query",
    hdrs = ["custom_query.h"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "mock_custom_query",
    hdrs = ["mock_custom_query.h"],
    deps = [
        ":custom_query",
    ],
)
