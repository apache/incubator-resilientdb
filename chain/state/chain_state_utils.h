#pragma once

#include "chain/state/chain_state.h"
#include "platform/config/resdb_config_utils.h"

namespace resdb {

std::unique_ptr<ChainState> NewState(const std::string& cert_file,
                                     const ResConfigData& config_data);

}
