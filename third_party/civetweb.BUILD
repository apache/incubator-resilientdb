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

cc_library(
    name = "civetweb",
    srcs = [
        "src/CivetServer.cpp",
        "src/civetweb.c",
        "src/response.inl",
    ],
    hdrs = [
        "include/CivetServer.h",
        "include/civetweb.h",
    ],
    copts = [
        "-DUSE_IPV6",
        "-DNDEBUG",
        "-DNO_CGI",
        "-DNO_CACHING",
        "-DNO_SSL",
        "-DNO_FILES",
    ],
    textual_hdrs = [
        "src/md5.inl",
        "src/handle_form.inl",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
)
