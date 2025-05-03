package(default_visibility = ["//visibility:public"])

cc_library(
    name = "transaction_manager",
    srcs = ["transaction_manager.cpp"],
    hdrs = ["transaction_manager.h"],
    deps = [
        "//chain/storage",
        "//common:comm",
        "//platform/proto:resdb_cc_proto",
    ],
)

cc_library(
    name = "mock_transaction_manager",
    hdrs = ["mock_transaction_manager.h"],
    deps = [
        ":transaction_manager",
    ],
)

cc_library(
    name = "custom_query",
    hdrs = ["custom_query.h"],
    deps = [],
)
