package(default_visibility = ["//visibility:public"])

cc_library(
    name = "resdb_kv_client",
    srcs = ["resdb_kv_client.cpp"],
    hdrs = ["resdb_kv_client.h"],
    deps = [
        "//platform/interface:resdb_transaction_proposer",
        "//service/kv_service/proto:kv_server_cc_proto",
    ],
)
