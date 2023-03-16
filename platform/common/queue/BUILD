package(default_visibility = ["//visibility:public"])

cc_library(
    name = "blocking_queue",
    hdrs = [
        "blocking_queue.h",
    ],
)

cc_library(
    name = "lock_free_queue",
    hdrs = [
        "lock_free_queue.h",
    ],
    deps = [
        "//common:boost_lockfree",
        "//common:comm",
    ],
)

cc_test(
    name = "lock_free_queue_test",
    srcs = ["lock_free_queue_test.cpp"],
    deps = [
        ":lock_free_queue",
        "//common/test:test_main",
    ],
)

cc_library(
    name = "batch_queue",
    hdrs = [
        "batch_queue.h",
    ],
    deps = [
        "//common:comm",
    ],
)

cc_test(
    name = "batch_queue_test",
    srcs = ["batch_queue_test.cpp"],
    deps = [
        ":batch_queue",
        "//common/test:test_main",
    ],
)
