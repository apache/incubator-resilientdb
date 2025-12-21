#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#


licenses(["notice"])
exports_files(["LICENSE"])

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "license",
    srcs = ["COPYING"],
)

genrule(
    name = "snappy_stubs_public_h",
    srcs = [
        "snappy-stubs-public.h.in",
    ],
    outs = [
        "snappy-stubs-public.h",
    ],
    cmd =
        "sed 's/@ac_cv_have_stdint_h@/1/g' $(<) | " +
        "sed 's/@ac_cv_have_stddef_h@/1/g' | " +
        "sed 's/@ac_cv_have_sys_uio_h@/1/g' | " +
        "sed 's/@SNAPPY_MAJOR@/1/g' | " +
        "sed 's/@SNAPPY_MINOR@/1/g' | " +
        "sed s/'$${HAVE_SYS_UIO_H_01}'/1/g | " +
        "sed s/'$${HAVE_STDINT_H_01}'/1/g  | " +
        "sed s/'$${HAVE_STDDEF_H_01}'/1/g  | " +
        "sed 's/@SNAPPY_PATCHLEVEL@/3/g' > $(@)",
)

cc_library(
    name = "snappy",
    srcs = [
        "snappy.cc",
        "snappy-c.cc",
        "snappy-sinksource.cc",
        "snappy-stubs-internal.cc",
    ],
    hdrs = [
        "snappy.h",
        "snappy-c.h",
        "snappy-internal.h",
        "snappy-sinksource.h",
        "snappy-stubs-internal.h",
        ":snappy-stubs-public.h",
    ],
    copts = [
        "-Wno-non-virtual-dtor",
        "-Wno-unused-variable",
        "-Wno-implicit-fallthrough",
        "-Wno-unused-function",
    ],
    includes = ["."],
)
