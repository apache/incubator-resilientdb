package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utxo_client",
    srcs = ["utxo_client.cpp"],
    hdrs = ["utxo_client.h"],
    deps = [
        "//platform/interface:resdb_transaction_proposer",
        "//service/utxo/proto:rpc_cc_proto",
        "//service/utxo/proto:utxo_cc_proto",
    ],
)
