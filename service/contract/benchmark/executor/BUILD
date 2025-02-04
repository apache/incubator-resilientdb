package(default_visibility = ["//service/contract/benchmark:__subpackages__"])

cc_library(
    name = "benchmark_executor",
    hdrs = ["benchmark_executor.h"],
    deps = [
        "//common/utils",
        "//service/contract/executor/common:contract_execute_info",
        "//service/contract/executor/common:utils",
    ],
)

cc_library(
    name = "occ_executor",
    srcs = ["occ_executor.cpp"],
    hdrs = ["occ_executor.h"],
    deps = [
        ":benchmark_executor",
        "//common/utils",
        "//service/contract/executor/manager:contract_manager",
        "//service/contract/executor/manager:leveldb_d_storage",
        "//service/contract/executor/manager:leveldb_storage",
    ],
)

cc_library(
    name = "bam_executor",
    srcs = ["bam_executor.cpp"],
    hdrs = ["bam_executor.h"],
    deps = [
        ":benchmark_executor",
        "//common/utils",
        "//service/contract/executor/x_manager:contract_manager",
        "//service/contract/executor/x_manager:leveldb_d_storage",
        "//service/contract/executor/x_manager:leveldb_storage",
    ],
)
