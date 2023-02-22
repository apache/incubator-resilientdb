licenses(["notice"])

genrule(
    name = "build_version",
    srcs = glob([".git/**/*"]) + [
        "util/build_version.cc.in",
    ],
    outs = [
        "util/build_version.cc",
    ],
    cmd = "grep -v 'define HAS_GIT_CHANGES' $(<) | " +
          "sed 's/@ROCKSDB_PLUGIN_BUILTINS@//g' | " +
          "sed 's/@ROCKSDB_PLUGIN_EXTERNS@//g'  > $(@)",
)

cc_library(
    name = "rocksdb",
    srcs =
        glob(
            [
                "**/*.h",
                "**/*.cc",
                "utilities/*.cc",
            ],
            exclude = [
                "java/**/*",
                "fuzz/**",
                "**/*test.cc",
                "**/*bench.cc",
                "microbench/**",
                "third-party/**",
                "util/log_write_bench.cc",
                "db/forward_iterator_bench.cc",
                "db/db_test2.cc",
                "db_stress_tool/*.cc",
                "tools/**/*.cc",
                "**/**/*example.cc",
            ],
        ) + [":build_version"],
    hdrs = glob([
        "include/rocksdb/**/*.h",
    ]),
    copts = [
        "-DGFLAGS=gflags",
        "-DJEMALLOC_NO_DEMANGLE",
        "-DOS_LINUX",
        "-DSNAPPY",
        "-DZLIB",
        "-fno-omit-frame-pointer",
        "-momit-leaf-frame-pointer",
        "-pthread",
        "-Werror",
        "-Wsign-compare",
        "-Wshadow",
        "-Wno-unused-parameter",
        "-Wno-unused-variable",
        "-Woverloaded-virtual",
        "-Wnon-virtual-dtor",
        "-Wno-missing-field-initializers",
        "-std=c++17",
        "-W",
        "-Wextra",
        "-Wall",
        "-Wsign-compare",
        "-Wno-unused-lambda-capture",
        "-Wno-invalid-offsetof",
        "-Wunused-variable",
        "-Wshadow",
        "-O3",
    ],
    defines = [
        "ROCKSDB_FALLOCATE_PRESENT",
        "ROCKSDB_LIB_IO_POSIX",
        "ROCKSDB_MALLOC_USABLE_SIZE",
        "ROCKSDB_PLATFORM_POSIX",
        "ROCKSDB_SUPPORT_THREAD_LOCAL",
        "DISABLE_JEMALLOC",
    ],
    includes = [
        ".",
        "include",
        "util",
    ],
    linkopts = [
        "-lm",
        "-ldl",
        "-lpthread",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//external:glog",
        "//external:gtest",
        "//external:snappy",
        "//external:zlib",
    ],
)
