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
    srcs = ["LICENSE"],
)

# Main FAISS library
cc_library(
    name = "faiss",
    srcs = glob(
        [
            "faiss/**/*.cpp",
            "faiss/**/*.c",
        ],
        exclude = [
            "**/*_test.cpp",
            "**/*_test.c",
            "**/*_bench.cpp",
            "**/*_bench.c",
            "**/*_example.cpp",
            "**/*_example.c",
            "**/*_demo.cpp",
            "**/*_demo.c",
            "**/*_main.cpp",
            "**/*_main.c",
            "faiss/gpu/**/*.cpp",  # Exclude GPU code for now
            "faiss/gpu/**/*.c",
            "faiss/gpu/**/*.cu",   # CUDA files
            "faiss/gpu/**/*.h",
            "faiss/gpu/**/*.hpp",
            "faiss/python/**/*.cpp",  # Exclude Python bindings
            "faiss/python/**/*.c",
            "faiss/python/**/*.h",
            "faiss/python/**/*.hpp",
        ],
    ),
    hdrs = glob(
        [
            "faiss/**/*.h",
            "faiss/**/*.hpp",
        ],
        exclude = [
            "**/*_test.h",
            "**/*_test.hpp",
            "**/*_bench.h",
            "**/*_bench.hpp",
            "**/*_example.h",
            "**/*_example.hpp",
            "**/*_demo.h",
            "**/*_demo.hpp",
            "**/*_main.h",
            "**/*_main.hpp",
            "faiss/gpu/**/*.h",
            "faiss/gpu/**/*.hpp",
            "faiss/python/**/*.h",
            "faiss/python/**/*.hpp",
        ],
    ),
    copts = [
        "-std=c++11",
        "-DFINTEGER=int",
        "-DUSE_BLAS",
        "-DUSE_OPENMP",
        "-fopenmp",
        "-Wno-unused-variable",
        "-Wno-unused-function",
        "-Wno-sign-compare",
        "-Wno-deprecated-declarations",
        "-Wno-unused-parameter",
        "-Wno-missing-field-initializers",
    ],
    includes = [
        ".",
        "faiss",
    ],
    deps = [
        "@com_zlib//:zlib",
    ],
    linkopts = [
        "-fopenmp",
        "-lm",
    ],
)

# FAISS C API library
cc_library(
    name = "faiss_c",
    srcs = [
        "faiss/c_api/Index_c.cpp",
        "faiss/c_api/IndexFlat_c.cpp",
        "faiss/c_api/IndexIVF_c.cpp",
        "faiss/c_api/IndexLSH_c.cpp",
        "faiss/c_api/IndexPQ_c.cpp",
        "faiss/c_api/IndexScalarQuantizer_c.cpp",
        "faiss/c_api/MetaIndexes_c.cpp",
        "faiss/c_api/VectorTransform_c.cpp",
        "faiss/c_api/clone_index_c.cpp",
        "faiss/c_api/error_c.cpp",
        "faiss/c_api/faiss_c.cpp",
        "faiss/c_api/index_factory_c.cpp",
        "faiss/c_api/index_io_c.cpp",
        "faiss/c_api/utils_c.cpp",
    ],
    hdrs = [
        "faiss/c_api/Index_c.h",
        "faiss/c_api/IndexFlat_c.h",
        "faiss/c_api/IndexIVF_c.h",
        "faiss/c_api/IndexLSH_c.h",
        "faiss/c_api/IndexPQ_c.h",
        "faiss/c_api/IndexScalarQuantizer_c.h",
        "faiss/c_api/MetaIndexes_c.h",
        "faiss/c_api/VectorTransform_c.h",
        "faiss/c_api/clone_index_c.h",
        "faiss/c_api/error_c.h",
        "faiss/c_api/faiss_c.h",
        "faiss/c_api/index_factory_c.h",
        "faiss/c_api/index_io_c.h",
        "faiss/c_api/utils_c.h",
    ],
    copts = [
        "-std=c++11",
        "-DFINTEGER=int",
        "-DUSE_BLAS",
        "-DUSE_OPENMP",
        "-fopenmp",
        "-Wno-unused-variable",
        "-Wno-unused-function",
        "-Wno-sign-compare",
        "-Wno-deprecated-declarations",
        "-Wno-unused-parameter",
        "-Wno-missing-field-initializers",
    ],
    includes = [
        ".",
        "faiss",
        "faiss/c_api",
    ],
    deps = [
        ":faiss",
        "@com_zlib//:zlib",
    ],
    linkopts = [
        "-fopenmp",
        "-lm",
    ],
)

# FAISS headers only target
cc_library(
    name = "faiss_headers",
    hdrs = glob([
        "faiss/**/*.h",
        "faiss/**/*.hpp",
    ]),
    includes = [
        "faiss",
    ],
    visibility = ["//visibility:public"],
)
