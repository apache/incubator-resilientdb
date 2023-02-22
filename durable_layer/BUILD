package(default_visibility = ["//visibility:public"])

cc_library(
    name = "leveldb_durable",
    srcs = ["leveldb_durable.cpp"],
    hdrs = ["leveldb_durable.h"],
    deps = [
        "//common:comm",
        "//config:resdb_config_utils",
        "//third_party:leveldb",
        "//third_party:rapidjson",
    ],
)

cc_test(
    name = "leveldb_durable_test",
    srcs = ["leveldb_durable_test.cpp"],
    deps = [
        "leveldb_durable",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "rocksdb_durable",
    srcs = ["rocksdb_durable.cpp"],
    hdrs = ["rocksdb_durable.h"],
    deps = [
        "//common:comm",
        "//config:resdb_config_utils",
        "//third_party:rocksdb",
    ],
)

cc_test(
    name = "rocksdb_durable_test",
    srcs = ["rocksdb_durable_test.cpp"],
    deps = [
        "rocksdb_durable",
        "//common/test:test_main",
    ],
)
