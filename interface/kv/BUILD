package(default_visibility = ["//visibility:public"])

cc_library(
    name = "kv_client",
    srcs = ["kv_client.cpp"],
    hdrs = ["kv_client.h"],
    deps = [
        "//interface/rdbc:transaction_constructor",
        "//proto/kv:kv_cc_proto",
    ],
)
