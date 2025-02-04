package(default_visibility = ["//service/contract/benchmark:__subpackages__"])

cc_library(
    name = "ycsb_workload",
    srcs = ["ycsb_workload.cpp"],
    hdrs = ["ycsb_workload.h"],
    deps = [
        "//third_party:ycsb_lib",
    ],
)

cc_library(
    name = "ycsb_benchmark",
    srcs = ["ycsb_benchmark.cpp"],
    hdrs = ["ycsb_benchmark.h"],
    deps = [
        ":ycsb_workload",
        "//service/contract/benchmark/ycsb/db:contract_db",
        "//third_party:ycsb_lib",
    ],
)

cc_binary(
    name = "occ_ycsb_main",
    srcs = ["occ_ycsb_main.cpp"],
    deps = [
        ":occ_contract",
        ":ycsb_benchmark",
    ],
)

cc_binary(
    name = "bam_ycsb_main",
    srcs = ["bam_ycsb_main.cpp"],
    deps = [
        ":bam_contract",
        ":ycsb_benchmark",
    ],
)

cc_library(
    name = "occ_contract",
    srcs = ["occ_contract.cpp"],
    hdrs = ["occ_contract.h"],
    deps = [
        "//common:comm",
        "//service/contract/benchmark/executor:occ_executor",
        "//service/contract/benchmark/ycsb/db:contract_db",
        "//service/contract/benchmark/ycsb/db:contract_executor",
    ],
)

cc_library(
    name = "bam_contract",
    srcs = ["bam_contract.cpp"],
    hdrs = ["bam_contract.h"],
    deps = [
        "//common:comm",
        "//service/contract/benchmark/executor:bam_executor",
        "//service/contract/benchmark/ycsb/db:contract_db",
        "//service/contract/benchmark/ycsb/db:contract_executor",
    ],
)
