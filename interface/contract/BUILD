package(default_visibility = ["//visibility:public"])

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
    ],
)
