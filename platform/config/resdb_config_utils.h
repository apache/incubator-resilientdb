/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <optional>

#include "platform/config/resdb_config.h"

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
