package(default_visibility = ["//visibility:public"])

cc_test(
    name = "resdb_test",
    srcs = ["resdb_test.cpp"],
    data = [
        "//test/test_data",
    ],
    deps = [
        "//client:resdb_user_client",
        "//common:comm",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//execution:transaction_executor_impl",
        "//ordering/pbft:consensus_service_pbft",
        "//server:resdb_server",
        "//test/proto:resdb_test_cc_proto",
    ],
)
