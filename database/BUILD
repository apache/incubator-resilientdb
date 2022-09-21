package(default_visibility = ["//visibility:public"])

cc_library(
    name = "txn_memory_db",
    srcs = ["txn_memory_db.cpp"],
    hdrs = ["txn_memory_db.h"],
    deps = [
        "//common:comm",
        "//proto:resdb_cc_proto",
    ],
)

cc_test(
    name = "txn_memory_db_test",
    srcs = ["txn_memory_db_test.cpp"],
    deps = [
        ":txn_memory_db",
        "//common/test:test_main",
    ],
)
