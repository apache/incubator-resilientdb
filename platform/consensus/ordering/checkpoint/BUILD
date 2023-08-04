package(default_visibility = ["//platform/consensus:__subpackages__"])

cc_library(
    name = "checkpoint",
    hdrs = ["checkpoint.h"],
    deps = [
    ],
)

cc_library(
    name = "mock_checkpoint",
    testonly = True,
    hdrs = ["mock_checkpoint.h"],
    deps = [
        ":checkpoint",
        "//common/test",
    ],
)
