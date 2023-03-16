package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "key_generator_tools",
    srcs = ["key_generator_tools.cpp"],
    deps = [
        "//common/crypto:key_generator",
    ],
)

cc_binary(
    name = "certificate_tools",
    srcs = ["certificate_tools.cpp"],
    deps = [
        "//common/crypto:key_generator",
        "//common/crypto:signature_verifier",
    ],
)

cc_binary(
    name = "certificate_tools_test",
    srcs = ["certificate_tools_test.cpp"],
    deps = [
        "//common/crypto:signature_verifier",
    ],
)

cc_binary(
    name = "resdb_state_accessor_tools",
    srcs = ["resdb_state_accessor_tools.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/interface:resdb_state_accessor",
    ],
)

cc_binary(
    name = "resdb_txn_accessor_tools",
    srcs = ["resdb_txn_accessor_tools.cpp"],
    deps = [
        "//platform/config:resdb_config_utils",
        "//platform/interface:resdb_txn_accessor",
    ],
)

#py_binary(
#    name = "generate_region_config",
#    srcs = ["generate_region_config.py"],
#    deps = [
#        "//platform/proto:replica_info_py_proto",
#    ],
#)

#py_binary(
#    name = "generate_mulregion_config",
#    srcs = ["generate_mulregion_config.py"],
#    deps = [
#        "//platform/proto:replica_info_py_proto",
#    ],
#)
