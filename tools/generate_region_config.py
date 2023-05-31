import os
import json
import sys
from proto.replica_info_pb2 import ResConfigData,ReplicaInfo
from google.protobuf.json_format import MessageToJson
from google.protobuf.json_format import Parse, ParseDict


def GenerateJsonConfig(file_name, output_file):
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

if __name__ == "__main__":
    GenerateJsonConfig(sys.argv[1], sys.argv[2])
