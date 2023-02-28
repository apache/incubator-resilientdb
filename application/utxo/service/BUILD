package(default_visibility = ["//application/utxo/server:__pkg__"])

cc_library(
    name = "utxo_executor",
    srcs = ["utxo_executor.cpp"],
    hdrs = ["utxo_executor.h"],
    deps = [
        "//application/utxo/proto:config_cc_proto",
        "//application/utxo/proto:rpc_cc_proto",
        "//application/utxo/utxo:transaction",
        "//application/utxo/utxo:wallet",
        "//config:resdb_config_utils",
        "//execution:custom_query",
        "//ordering/pbft:consensus_service_pbft",
    ],
)

cc_test(
    name = "utxo_executor_test",
    srcs = ["utxo_executor_test.cpp"],
    deps = [
        ":utxo_executor",
        "//common/test:test_main",
        "//crypto:hash",
        "//crypto:key_generator",
        "//crypto:signature_utils",
    ],
)
