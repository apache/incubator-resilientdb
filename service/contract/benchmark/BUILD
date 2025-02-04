package(default_visibility = ["//visibility:private"])

cc_library(
    name = "occ_bm",
    srcs = ["occ_bm.cpp"],
    hdrs = ["occ_bm.h"],
    deps = [
        "//common/utils",
        "//service/contract/benchmark/generator:string_generator",
        "//service/contract/benchmark/generator:zipfian_generator",
        "//service/contract/executor/manager:contract_manager",
    ],
)

cc_library(
    name = "bam_bm",
    srcs = ["bam_bm.cpp"],
    hdrs = ["bam_bm.h"],
    deps = [
        "//common/utils",
        "//service/contract/benchmark/generator:string_generator",
        "//service/contract/benchmark/generator:zipfian_generator",
        "//service/contract/executor/x_manager:contract_manager",
    ],
)

cc_binary(
    name = "occ_trans_performance",
    srcs = ["occ_trans_performance.cpp"],
    data = [
        "//service/contract/benchmark/data:ERC20.json",
    ],
    deps = [
        ":occ_bm",
        "//common/utils",
    ],
)

cc_binary(
    name = "occ_smallbank_performance",
    srcs = ["occ_smallbank_performance.cpp"],
    data = [
        "//service/contract/benchmark/data:smallbank.json",
    ],
    deps = [
        ":occ_bm",
        "//common/utils",
        "//service/contract/executor/x_manager:data_storage",
        "//service/contract/executor/x_manager:leveldb_storage",
    ],
)

cc_binary(
    name = "bam_trans_performance",
    srcs = ["bam_trans_performance.cpp"],
    data = [
        "//service/contract/benchmark/data:ERC20.json",
    ],
    deps = [
        ":bam_bm",
        "//common/utils",
        "//service/contract/executor/x_manager:data_storage",
    ],
)

cc_binary(
    name = "bam_smallbank_performance",
    srcs = ["bam_smallbank_performance.cpp"],
    data = [
        "//service/contract/benchmark/data:smallbank.json",
    ],
    deps = [
        ":bam_bm",
        "//common/utils",
        "//service/contract/executor/x_manager:leveldb_storage",
    ],
)
