package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "contract_tools",
    srcs = ["contract_tools.cpp"],
    deps = [
        "//common:boost_comm",
        "//common:json",
        "//platform/config:resdb_config_utils",
        "//service/contract/interface:contract_client",
    ],
)
