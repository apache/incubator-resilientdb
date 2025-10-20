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

workspace(name = "com_resdb_nexres")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "hedron_compile_commands",
    #Replace the commit hash (4f28899228fb3ad0126897876f147ca15026151e) with the latest commit hash from the repo
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/4f28899228fb3ad0126897876f147ca15026151e.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-4f28899228fb3ad0126897876f147ca15026151e",
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()
load("@hedron_compile_commands//:workspace_setup_transitive.bzl", "hedron_compile_commands_setup_transitive")
hedron_compile_commands_setup_transitive()
load("@hedron_compile_commands//:workspace_setup_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive")
hedron_compile_commands_setup_transitive_transitive()
load("@hedron_compile_commands//:workspace_setup_transitive_transitive_transitive.bzl", "hedron_compile_commands_setup_transitive_transitive_transitive")
hedron_compile_commands_setup_transitive_transitive_transitive()

load("//:repositories.bzl", "nexres_repositories")

nexres_repositories()

http_archive(
    name = "rules_foreign_cc",
    sha256 = "69023642d5781c68911beda769f91fcbc8ca48711db935a75da7f6536b65047f",
    strip_prefix = "rules_foreign_cc-0.6.0",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.6.0.tar.gz",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

http_archive(
    name = "rules_proto",
    sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
    strip_prefix = "rules_proto-4.0.0",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

http_archive(
    name = "rules_python",
    sha256 = "ffc7b877c95413c82bfd5482c017edcf759a6250d8b24e82f41f3c8b8d9e287e",
    strip_prefix = "rules_python-0.19.0",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.19.0/rules_python-0.19.0.tar.gz",
)

load("@rules_python//python:pip.bzl", "pip_install")
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

load("@rules_proto_grpc//:repositories.bzl", "rules_proto_grpc_repos", "rules_proto_grpc_toolchains")

rules_proto_grpc_toolchains()

rules_proto_grpc_repos()

bind(
    name = "gtest",
    actual = "@com_google_googletest//:gtest",
)

bind(
    name = "glog",
    actual = "@com_github_google_glog//:glog",
)

http_archive(
    name = "com_github_google_glog",
    sha256 = "21bc744fb7f2fa701ee8db339ded7dce4f975d0d55837a97be7d46e8382dea5a",
    strip_prefix = "glog-0.5.0",
    urls = ["https://github.com/google/glog/archive/v0.5.0.zip"],
)

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/protocolbuffers/protobuf",
    tag = "v3.10.0",
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

all_content = """filegroup(name = "all_srcs", srcs = glob(["**"]), visibility = ["//visibility:public"])"""

# buildifier is written in Go and hence needs rules_go to be built.
# See https://github.com/bazelbuild/rules_go for the up to date setup instructions.
http_archive(
    name = "io_bazel_rules_go",
    sha256 = "d6b2513456fe2229811da7eb67a444be7785f5323c6708b38d851d2b51e54d83",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.30.0/rules_go-v0.30.0.zip",
        "https://github.com/bazelbuild/rules_go/releases/download/v0.30.0/rules_go-v0.30.0.zip",
    ],
)

load("@io_bazel_rules_go//go:deps.bzl", "go_rules_dependencies")

go_rules_dependencies()

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains")

go_register_toolchains(version = "1.19.5")

http_archive(
    name = "bazel_gazelle",
    sha256 = "de69a09dc70417580aabf20a28619bb3ef60d038470c7cf8442fafcf627c21cb",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-gazelle/releases/download/v0.24.0/bazel-gazelle-v0.24.0.tar.gz",
        "https://github.com/bazelbuild/bazel-gazelle/releases/download/v0.24.0/bazel-gazelle-v0.24.0.tar.gz",
    ],
)

load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies")

gazelle_dependencies()

http_archive(
    name = "com_github_bazelbuild_buildtools",
    sha256 = "518b2ce90b1f8ad7c9a319ca84fd7de9a0979dd91e6d21648906ea68faa4f37a",
    strip_prefix = "buildtools-5.0.1",
    urls = [
        "https://github.com/bazelbuild/buildtools/archive/refs/tags/5.0.1.zip",
    ],
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()

http_archive(
    name = "net_zlib_zlib",
    build_file = "@com_resdb_nexres//third_party:z.BUILD",
    sha256 = "91844808532e5ce316b3c010929493c0244f3d37593afd6de04f71821d5136d9",
    strip_prefix = "zlib-1.2.12",
    urls = [
        "https://storage.googleapis.com/bazel-mirror/zlib.net/zlib-1.2.12.tar.gz",
    ],
)

http_archive(
    name = "com_google_leveldb",
    build_file = "@com_resdb_nexres//third_party:leveldb.BUILD",
    sha256 = "a6fa7eebd11de709c46bf1501600ed98bf95439d6967963606cc964931ce906f",
    strip_prefix = "leveldb-1.23",
    url = "https://github.com/google/leveldb/archive/refs/tags/1.23.zip",
)

bind(
    name = "zstd",
    actual = "//third_party:zstd",
)

http_archive(
    name = "com_facebook_zstd",
    build_file_content = all_content,
    strip_prefix = "zstd-1.5.2",
    url = "https://github.com/facebook/zstd/archive/refs/tags/v1.5.2.zip",
)

http_archive(
    name = "com_github_facebook_rocksdb",
    build_file = "@com_resdb_nexres//third_party:rocksdb.BUILD",
    sha256 = "928cbd416c0531e9b2e7fa74864ce0d7097dca3f5a8c31f31459772a28dbfcba",
    strip_prefix = "rocksdb-7.2.2",
    url = "https://github.com/facebook/rocksdb/archive/refs/tags/v7.2.2.zip",
)

bind(
    name = "snappy",
    actual = "@com_google_snappy//:snappy",
)

http_archive(
    name = "com_google_snappy",
    build_file = "@com_resdb_nexres//third_party:snappy.BUILD",
    sha256 = "e170ce0def2c71d0403f5cda61d6e2743373f9480124bcfcd0fa9b3299d428d9",
    strip_prefix = "snappy-1.1.9",
    url = "https://github.com/google/snappy/archive/refs/tags/1.1.9.zip",
)

bind(
    name = "zlib",
    actual = "@com_zlib//:zlib",
)

http_archive(
    name = "com_zlib",
    build_file = "@com_resdb_nexres//third_party:zlib.BUILD",
    sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
    strip_prefix = "zlib-1.2.11",
    url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
)

http_archive(
    name = "pybind11_bazel",
    strip_prefix = "pybind11_bazel-2.11.1.bzl.1",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/refs/tags/v2.11.1.bzl.1.zip"]
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    sha256 = "8ff2fff22df038f5cd02cea8af56622bc67f5b64534f1b83b9f133b8366acff2",
    strip_prefix = "pybind11-2.6.2",
    urls = ["https://github.com/pybind/pybind11/archive/v2.6.2.tar.gz"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(
    name = "local_config_python",
    python_version = "3",
)

http_archive(
    name = "nlohmann_json",
    build_file = "@com_resdb_nexres//third_party:json.BUILD",  # see below
    sha256 = "4cf0df69731494668bdd6460ed8cb269b68de9c19ad8c27abc24cd72605b2d5b",
    strip_prefix = "json-3.9.1",
    urls = ["https://github.com/nlohmann/json/archive/v3.9.1.tar.gz"],
)

http_archive(
    name = "com_crowcpp_crow",
    build_file = "//third_party:crow.BUILD",
    sha256 = "f95128a8976fae6f2922823e07da59edae277a460776572a556a4b663ff5ee4b",
    strip_prefix = "Crow-1.0-5",
    url = "https://github.com/CrowCpp/Crow/archive/refs/tags/v1.0+5.zip",
)

bind(
    name = "asio",
    actual = "@com_chriskohlhoff_asio//:asio",
)

http_archive(
    name = "com_chriskohlhoff_asio",
    build_file = "//third_party:asio.BUILD",
    sha256 = "babcdfd2c744905a73d20de211b51367bda0d5200f11d654c4314b909d8c963c",
    strip_prefix = "asio-asio-1-26-0",
    url = "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-26-0.zip",
)

http_archive(
    name = "com_google_benchmark",
    sha256 = "6bc180a57d23d4d9515519f92b0c83d61b05b5bab188961f36ac7b06b0d9e9ce",
    strip_prefix = "benchmark-1.8.3",
    urls = ["https://github.com/google/benchmark/archive/v1.8.3.tar.gz"],
)
