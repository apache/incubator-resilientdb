package(default_visibility = ["//visibility:public"])

cc_library(
    name = "glog",
    deps = [
        "@com_github_google_glog//:glog",
    ],
)

cc_library(
    name = "tpcc_executor",
    srcs = ["tpcc_executor.cpp"],
    hdrs = ["tpcc_executor.h"], 
    deps = [
        ":glog",
        "//platform/proto:resdb_cc_proto",
    ],
    copts = [
        "-I/usr/include",  # Include path for SQLite3 headers
    ],
    linkopts = [
        "-L/usr/lib",  # Library path for linking
        "-lsqlite3",   # Link against the SQLite3 library
    ],
)

cc_library(
    name = "transaction_manager",
    srcs = ["transaction_manager.cpp"],
    hdrs = ["transaction_manager.h"],
    deps = [
        ":tpcc_executor",
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
