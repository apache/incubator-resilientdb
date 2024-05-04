#
# Copyright (c) 2019-2022 ExpoLab, UC Davis
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
#


package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test",
    hdrs = [
        "test_macros.h",
    ],
    deps = [
        "//common:comm",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "test_main",
    defines = ["TEST_MODE"],
    deps = [
        ":test",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
    visibility = ["//visibility:private"],
)

cc_proto_library(
    name = "test_cc_proto",
    visibility = ["//visibility:private"],
    deps = [":test_proto"],
)

cc_test(
    name = "json_test",
    srcs = ["json_test.cpp"],
    deps = [
        ":test_cc_proto",
        ":test_main",
    ],
)
