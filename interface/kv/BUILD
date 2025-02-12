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

cc_library(
    name = "contract_client",
    srcs = ["contract_client.cpp"],
    hdrs = ["contract_client.h"],
    deps = [
        "//common:json",
        "//interface/rdbc:transaction_constructor",
        "//proto/contract:account_cc_proto",
        "//proto/contract:contract_cc_proto",
        "//proto/contract:rpc_cc_proto",
        "//proto/kv:kv_cc_proto",
    ],
)
