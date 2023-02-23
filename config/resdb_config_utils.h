/*
 * Copyright (c) 2019-2022 ExpoLab, UC Davis
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <optional>

#include "config/resdb_config.h"

namespace resdb {

std::vector<ReplicaInfo> ReadConfig(const std::string& file_name);
ReplicaInfo GenerateReplicaInfo(int id, const std::string& ip, int port);

typedef std::function<std::unique_ptr<ResDBConfig>(
    const ResConfigData& config_data, const ReplicaInfo& self_info,
    const KeyInfo& private_key, const CertificateInfo& public_key_cert_info)>
    ConfigGenFunc;

std::unique_ptr<ResDBConfig> GenerateResDBConfig(
    const std::string& config_file, const std::string& private_key_file,
    const std::string& cert_file,
    std::optional<ReplicaInfo> self_info = std::nullopt,
    std::optional<ConfigGenFunc> = std::nullopt);

ResDBConfig GenerateResDBConfig(const std::string& config_file);
}  // namespace resdb
