import os
import json
import sys
from proto.replica_info_pb2 import ResConfigData,ReplicaInfo
from google.protobuf.json_format import MessageToJson
from google.protobuf.json_format import Parse, ParseDict


def GenerateJsonConfig(files):
    region_id = 1
    tmp_config={}
    for file_name in files:
        with open(file_name) as f:
            for line in f.readlines():
                if len(line.split()) == 0:
                        continue
                info={}
                data = line.split()
                info["id"] = data[0]
                info["ip"] = data[1]
                info["port"] = data[2]
                if len(data) > 3:
                    region_id = data[3]
                if region_id not in tmp_config:
                        tmp_config[region_id] = []

                tmp_config[region_id].append({"replica_info":info})
            region_id=region_id+1

    for idx in range(1,region_id):
        config_data=ResConfigData() 
        reg_id=1
        for region_data in tmp_config.values():
            region = config_data.region.add()
            for rep in region_data:
                replica = Parse(json.dumps(rep["replica_info"]), ReplicaInfo())
                region.replica_info.append(replica)
            region.region_id=reg_id
            reg_id=reg_id+1
        config_data.self_region_id=idx
     
        json_obj = MessageToJson(config_data)
        with open("server_region{}.config_json".format(idx),"w") as f:
            f.write(json_obj)

if __name__ == "__main__":
    files=[]
    for x in sys.argv[1:]:
        files.append(x)

    GenerateJsonConfig(files)

