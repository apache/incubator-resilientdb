cc_library(
    name = "leveldb",
    srcs = glob(
        ["**/*.cc"],
        exclude = [
            "doc/**",
            "**/*_test.cc",
            "db/leveldbutil.cc",
            "db/db_bench.cc",
            "benchmarks/*.cc",
            "util/env_windows.cc",
            "util/testutil.cc",
        ],
    ),
    hdrs = glob(
        ["**/*.h"],
        exclude = [
          "doc/**",
          "util/testutil.h",
          ],
    ),
    copts = [
        "-std=c++17",
    ],
    defines = [
        "LEVELDB_PLATFORM_POSIX=1",
        "LEVELDB_IS_BIG_ENDIAN=0",
    ],
    includes = [
        ".",
        "include",
    ],
    visibility = ["//visibility:public"],
)
