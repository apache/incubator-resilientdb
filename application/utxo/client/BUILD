package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utxo_client",
    srcs = ["utxo_client.cpp"],
    hdrs = ["utxo_client.h"],
    deps = [
        "//application/utxo/proto:rpc_cc_proto",
        "//application/utxo/proto:utxo_cc_proto",
        "//client:resdb_user_client",
    ],
)
