package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    hdrs = ["utils.h"],
    deps = [
        "//third_party:evm_lib",
    ],
)

cc_library(
    name = "contract_execute_info",
    hdrs = ["contract_execute_info.h"],
    deps = [
        ":utils",
        "//service/contract/proto:func_params_cc_proto",
    ],
)
