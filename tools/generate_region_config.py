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

import os
import json
import sys
import re
from platform.proto.replica_info_pb2 import ResConfigData,ReplicaInfo
from google.protobuf.json_format import MessageToJson
from google.protobuf.json_format import Parse, ParseDict


def GenerateJsonConfig(file_name, output_file, template_file):
    config_data=ResConfigData() 
    tmp_config={}
    with open(file_name) as f:
        for line in f.readlines():
            if len(line.split()) == 0:
                    continue
            info={}
            data = line.split()
            info["id"] = data[0]
            info["ip"] = data[1]
            info["port"] = data[2]
            region_id = 0
            if len(data) > 3:
                region_id = data[3]
            if region_id not in tmp_config:
                    tmp_config[region_id] = []

            tmp_config[region_id].append({"replica_info":info})

    config = []
    for region_data in tmp_config.values():
        region = config_data.region.add()
        for rep in region_data:
            replica = Parse(json.dumps(rep["replica_info"]), ReplicaInfo())
            region.replica_info.append(replica)

    json_obj = MessageToJson(config_data)
    with open(output_file,"w") as f:
        f.write(json_obj)

    if template_file:
      old_json = None
      with open(output_file) as f:
        lines=f.readlines()
        for l in lines:
          l=l.strip()
        s=''.join(lines)
        old_json=json.loads(s)

      template_json = {}
      with open(template_file, 'r', encoding='utf-8') as f:
        content = f.read()

      content = re.sub(r'/\*(.*?)\*/', '', content, flags=re.DOTALL)

      lines = content.splitlines()
      clean_lines = []
      for line in lines:
          line = re.sub(r'//.*', '', line) 
          if line.strip(): 
              clean_lines.append(line)

      clean_content = '\n'.join(clean_lines)

      # 解析清理后的 JSON
      template_json = json.loads(clean_content)

      for (k,v) in template_json.items():
        old_json[k] = v

      with open(output_file,"w") as f:
        json.dump(old_json, f)

if __name__ == "__main__":
    template_config = None
    if len(sys.argv)>3:
      template_config = sys.argv[3]
    print("generate json config:",template_config)
    GenerateJsonConfig(sys.argv[1], sys.argv[2], template_config)
