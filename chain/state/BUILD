package(default_visibility = ["//visibility:public"])

cc_library(
    name = "key_value_storage",
    srcs = ["key_value_storage.cpp"],
    hdrs = ["key_value_storage.h"],
    deps = [
        "//chain/storage:storage",
        "//common:comm",
    ],
)

cc_test(
    name = "key_value_storage_test",
    srcs = ["key_value_storage_test.cpp"],
    deps = [
        ":key_value_storage",
        "//common/test:test_main",
    ],
)
