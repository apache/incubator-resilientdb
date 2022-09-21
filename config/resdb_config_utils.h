#pragma once

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
