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

iplist=(
127.0.0.1
127.0.0.1
127.0.0.1
127.0.0.1
127.0.0.1
)

WORKSPACE=$PWD
CERT_PATH=$PWD/service/tools/data/cert/
CONFIG_PATH=$PWD/service/tools/config/
PORT_BASE=20000
CLIENT_NUM=1
export LEARNER_CONFIG_PATH=$PWD/platform/learner/learner.config

./service/tools/config/generate_config.sh ${WORKSPACE} ${CERT_PATH} ${CERT_PATH} ${CONFIG_PATH} ${CERT_PATH} ${CLIENT_NUM} ${PORT_BASE} ${iplist[@]} 

python3 - "$CONFIG_PATH/server/server.config" <<'PY'
import json
import os
import sys

cfg_path = sys.argv[1]
with open(cfg_path) as f:
    cfg = json.load(f)

learner_config_path = os.environ.get("LEARNER_CONFIG_PATH")
if learner_config_path and os.path.exists(learner_config_path):
    with open(learner_config_path) as lf:
        learner_cfg = json.load(lf)
    learner_info = {
        "id": int(learner_cfg.get("node_id", 0)),
        "ip": learner_cfg.get("ip", "127.0.0.1"),
        "port": int(learner_cfg.get("port", 0)),
    }
    cfg["learnerInfo"] = [learner_info]
    if "block_size" in learner_cfg:
        cfg["block_size"] = int(learner_cfg["block_size"])

with open(cfg_path, "w") as f:
    json.dump(cfg, f, indent=2)
PY
