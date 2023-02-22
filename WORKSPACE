load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

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

bind(
    name = "gtest",
    actual = "@com_google_googletest//:gtest",
)

http_archive(
    name = "com_google_googletest",
    sha256 = "ffa17fbc5953900994e2deec164bb8949879ea09b411e07f215bfbb1f87f4632",
    strip_prefix = "googletest-1.13.0",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip"],
)

http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
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

http_archive(
    name = "cryptopp",
    build_file_content = all_content,
    sha256 = "6055ab314ff4daae9490ddfb3fbcf107bc94a556401ed42f756fa5f7cd8c6510",
    strip_prefix = "cryptopp-CRYPTOPP_8_7_0",
    urls = ["https://github.com/weidai11/cryptopp/archive/refs/tags/CRYPTOPP_8_7_0.zip"],
)

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-20211102.0",
    urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.zip"],
)

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

_RULES_BOOST_COMMIT = "652b21e35e4eeed5579e696da0facbe8dba52b1f"

http_archive(
    name = "com_github_nelhage_rules_boost",
    strip_prefix = "rules_boost-96e9b631f104b43a53c21c87b01ac538ad6f3b48",

    # Replace the commit hash in both places (below) with the latest, rather than using the stale one here.
    # Even better, set up Renovate and let it do the work for you (see "Suggestion: Updates" in the README).
    url = "https://github.com/nelhage/rules_boost/archive/96e9b631f104b43a53c21c87b01ac538ad6f3b48.tar.gz",
    # When you first run this tool, it'll recommend a sha256 hash to put here with a message like: "DEBUG: Rule 'com_github_nelhage_rules_boost' indicated that a canonical reproducible form can be obtained by modifying arguments sha256 = ..."
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()

http_archive(
    name = "net_zlib_zlib",
    build_file = "//third_party:z.BUILD",
    sha256 = "91844808532e5ce316b3c010929493c0244f3d37593afd6de04f71821d5136d9",
    strip_prefix = "zlib-1.2.12",
    urls = [
        "https://zlib.net/zlib-1.2.12.tar.gz",
        "https://storage.googleapis.com/bazel-mirror/zlib.net/zlib-1.2.12.tar.gz",
    ],
)

#prometheus cpp client library

http_archive(
    name = "com_github_jupp0r_prometheus_cpp",
    build_file = "//third_party:prometheus.BUILD",
    sha256 = "281b6d9a26da35375c9958954e03616d71ea28d57ec193b0e75c3e10ff3da55d",
    strip_prefix = "prometheus-cpp-1.0.1",
    url = "https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v1.0.1.zip",
)

load("@com_github_jupp0r_prometheus_cpp//bazel:repositories.bzl", "prometheus_cpp_repositories")

http_archive(
    name = "com_google_leveldb",
    build_file = "//third_party:leveldb.BUILD",
    sha256 = "a6fa7eebd11de709c46bf1501600ed98bf95439d6967963606cc964931ce906f",
    strip_prefix = "leveldb-1.23",
    url = "https://github.com/google/leveldb/archive/refs/tags/1.23.zip",
)

bind(
    name = "date",
    actual = "@com_howardhinnant_date//:date",
)

http_archive(
    name = "com_howardhinnant_date",
    build_file = "//third_party:date.BUILD",
    sha256 = "f4300b96f7a304d4ef9bf6e0fa3ded72159f7f2d0f605bdde3e030a0dba7cf9f",
    strip_prefix = "date-3.0.1",
    url = "https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.1.zip",
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
    sha256 = "1607ed2e52efec86f02590efcab2614179df4ea73e52eba972507dc7fd08375d",
    strip_prefix = "asio-master",
    url = "https://github.com/chriskohlhoff/asio/archive/refs/heads/master.zip",
)

bind(
    name = "snappy",
    actual = "@com_google_snappy//:snappy",
)

http_archive(
    name = "com_google_snappy",
    build_file = "//third_party:snappy.BUILD",
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
    build_file = "//third_party:zlib.BUILD",
    sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
    strip_prefix = "zlib-1.2.11",
    url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
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

bind(
    name = "jemalloc",
    actual = "//third_party:jemalloc",
)

http_archive(
    name = "com_jemalloc",
    build_file_content = all_content,
    sha256 = "1f35888bad9fd331f5a03445bc1bff808a59378be61fef01e9736179d76f2fab",
    strip_prefix = "jemalloc-5.3.0",
    url = "https://github.com/jemalloc/jemalloc/archive/refs/tags/5.3.0.zip",
)

http_archive(
    name = "com_github_facebook_rocksdb",
    build_file = "//third_party:rocksdb.BUILD",
    sha256 = "928cbd416c0531e9b2e7fa74864ce0d7097dca3f5a8c31f31459772a28dbfcba",
    strip_prefix = "rocksdb-7.2.2",
    url = "https://github.com/facebook/rocksdb/archive/refs/tags/v7.2.2.zip",
)

http_archive(
    name = "eEVM",
    build_file = "//third_party:eEVM.BUILD",
    strip_prefix = "eEVM-main",
    url = "https://github.com/microsoft/eEVM/archive/refs/heads/main.zip",
)

http_archive(
    name = "pybind11_bazel",
    strip_prefix = "pybind11_bazel-master",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/master.zip"],
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
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
    build_file = "//third_party:json.BUILD",  # see below
    sha256 = "4cf0df69731494668bdd6460ed8cb269b68de9c19ad8c27abc24cd72605b2d5b",
    strip_prefix = "json-3.9.1",
    urls = ["https://github.com/nlohmann/json/archive/v3.9.1.tar.gz"],
)

http_archive(
    name = "rapidjson",
    build_file = "//third_party:rapidjson.BUILD",
    sha256 = "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab",
    strip_prefix = "rapidjson-1.1.0",
    url = "https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.zip",
)

http_archive(
    name = "civetweb",
    build_file = "//third_party:civetweb.BUILD",
    sha256 = "88574f0cffd6047e22fafa3bdc748dd878a4928409d4f880332e2b0f262b9f62",
    strip_prefix = "civetweb-1.15",
    url = "https://github.com/civetweb/civetweb/archive/refs/tags/v1.15.zip",
)
