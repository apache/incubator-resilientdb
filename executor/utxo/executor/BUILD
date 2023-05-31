package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utxo_executor",
    srcs = ["utxo_executor.cpp"],
    hdrs = ["utxo_executor.h"],
    deps = [
        "//executor/common:custom_query",
        "//executor/common:transaction_manager",
        "//executor/utxo/manager:transaction",
        "//executor/utxo/manager:wallet",
        "//platform/config:resdb_config_utils",
        "//proto/utxo:config_cc_proto",
        "//proto/utxo:rpc_cc_proto",
    ],
)

cc_test(
    name = "utxo_executor_test",
    srcs = ["utxo_executor_test.cpp"],
    deps = [
        ":utxo_executor",
        "//common/crypto:hash",
        "//common/crypto:key_generator",
        "//common/crypto:signature_utils",
        "//common/test:test_main",
    ],
)
