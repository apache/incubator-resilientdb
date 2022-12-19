package(default_visibility = ["//visibility:public"])

cc_library(
    name = "consensus_service_geo_pbft",
    srcs = ["consensus_service_geo_pbft.cpp"],
    hdrs = ["consensus_service_geo_pbft.h"],
    deps = [
        ":geo_pbft_commitment",
        "//execution:geo_transaction_executor",
        "//ordering/pbft:consensus_service_pbft",
    ],
)

cc_library(
    name = "geo_pbft_commitment",
    srcs = ["geo_pbft_commitment.cpp"],
    hdrs = ["geo_pbft_commitment.h"],
    deps = [
        ":hash_set",
        "//common:comm",
        "//config:resdb_config",
        "//execution:geo_global_executor",
        "//execution:system_info",
        "//ordering/pbft:transaction_utils",
        "//proto:resdb_cc_proto",
        "//server:resdb_replica_client",
        "//server:server_comm",
    ],
)

cc_library(
    name = "hash_set",
    hdrs = ["hash_set.h"],
)

cc_test(
    name = "geo_pbft_commitment_test",
    srcs = ["geo_pbft_commitment_test.cpp"],
    deps = [
        ":geo_pbft_commitment",
        "//common/test:test_main",
        "//config:resdb_config_utils",
        "//crypto:mock_signature_verifier",
        "//execution:mock_geo_global_executor",
        "//execution:mock_transaction_executor_impl",
        "//server:mock_resdb_replica_client",
    ],
)
