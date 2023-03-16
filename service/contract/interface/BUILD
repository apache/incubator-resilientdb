package(default_visibility = ["//visibility:public"])

cc_library(
    name = "contract_client",
    srcs = ["contract_client.cpp"],
    hdrs = ["contract_client.h"],
    deps = [
        "//common:json",
        "//platform/interface:resdb_transaction_proposer",
        "//service/contract/proto:account_cc_proto",
        "//service/contract/proto:contract_cc_proto",
        "//service/contract/proto:rpc_cc_proto",
    ],
)
