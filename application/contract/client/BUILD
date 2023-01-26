package(default_visibility = ["//visibility:public"])

cc_library(
    name = "contract_client",
    srcs = ["contract_client.cpp"],
    hdrs = ["contract_client.h"],
    deps = [
        "//application/contract/proto:account_cc_proto",
        "//application/contract/proto:contract_cc_proto",
        "//application/contract/proto:rpc_cc_proto",
        "//client:resdb_user_client",
    ],
)
