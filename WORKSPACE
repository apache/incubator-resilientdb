load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_foreign_cc",
    # TODO: Get the latest sha256 value from a bazel debug message or the latest
    #       release on the releases page: https://github.com/bazelbuild/rules_foreign_cc/releases
    #
    # sha256 = "...",
    strip_prefix = "rules_foreign_cc-50ee9979e60e8db38e10de45d2c60873a210bf55",
    url = "https://github.com/bazelbuild/rules_foreign_cc/archive/50ee9979e60e8db38e10de45d2c60873a210bf55.tar.gz",
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
    strip_prefix = "googletest-609281088cfefc76f9d0ce82e1ff6c30cc3591e5",
    urls = ["https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip"],
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
    sha256 = "5107c913c4682a07260f6a766aa1df8ec92a96c48d73994e023db3ac485bf532",
    strip_prefix = "cryptopp-CRYPTOPP_8_6_0",
    urls = ["https://github.com/weidai11/cryptopp/archive/refs/tags/CRYPTOPP_8_6_0.zip"],
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

go_register_toolchains(version = "1.17.2")

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
    sha256 = "c1b8b2adc3b4201683cf94dda7eef3fc0f4f4c0ea5caa3ed3feffe07e1fb5b15",
    strip_prefix = "rules_boost-%s" % _RULES_BOOST_COMMIT,
    urls = [
        "https://github.com/nelhage/rules_boost/archive/%s.tar.gz" % _RULES_BOOST_COMMIT,
    ],
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
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

http_archive(
    name = "com_github_jupp0r_prometheus_cpp",
    sha256 = "281b6d9a26da35375c9958954e03616d71ea28d57ec193b0e75c3e10ff3da55d",
    strip_prefix = "prometheus-cpp-1.0.1",
    url = "https://github.com/jupp0r/prometheus-cpp/archive/refs/tags/v1.0.1.zip",
)

load("@com_github_jupp0r_prometheus_cpp//bazel:repositories.bzl", "prometheus_cpp_repositories")

prometheus_cpp_repositories()

http_archive(
    name = "com_google_leveldb",
    build_file = "//third_party:leveldb.BUILD",
    sha256 = "a6fa7eebd11de709c46bf1501600ed98bf95439d6967963606cc964931ce906f",
    strip_prefix = "leveldb-1.23",
    url = "https://github.com/google/leveldb/archive/refs/tags/1.23.zip",
)

bind(
    name = "date",
    actual = "@com_google_date//:date",
)

http_archive(
    name = "com_google_date",
    build_file = "//third_party:date.BUILD",
    sha256 = "093f06859112120a58a820a81dc7f9048075e95a6634fa1f466a9a9bdda1e18e",
    strip_prefix = "date-master",
    url = "https://github.com/HowardHinnant/date/archive/refs/heads/master.zip",
)

'''
http_archive(
    name = "com_google_pistache",
    build_file = "//third_party:pistache.BUILD",
    sha256 = "0414c0a3b9e4ae01bae7c18d1b31f9ec9c041693959a66967a0d14a060481b0a",
    strip_prefix = "pistache-master",
    url = "https://github.com/pistacheio/pistache/archive/refs/heads/master.zip",
)

http_archive(
    name = "com_crowcpp_crow",
    build_file = "//third_party:crow.BUILD",
    sha256 = "b0da80870073112ef18e862cfd07936dd44c9a3311b64e471546fd9848aeeda7",
    strip_prefix = "Crow-master",
    url = "https://github.com/CrowCpp/Crow/archive/refs/heads/master.zip",
)

'''

bind(
    name = "asio",
    actual = "@com_chriskohlhoff_asio//:asio",
)

http_archive(
    name = "com_chriskohlhoff_asio",
    build_file = "//third_party:asio.BUILD",
    sha256 = "dbb066e63c1af407993cbfa6c1019671d4f7e020c182dde504a82e1104072627",
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
