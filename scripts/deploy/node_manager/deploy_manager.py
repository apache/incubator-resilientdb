from http.server import BaseHTTPRequestHandler
from urllib import parse
import subprocess
import json
import sys

from proto.replica_info_pb2 import ResConfigData,ReplicaInfo
from google.protobuf.json_format import MessageToJson
from google.protobuf.json_format import Parse, ParseDict

script_path="/home/ubuntu/nexres/deploy/node_manager"

def RunCmd(cmd):
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print("run:{}".format(cmd))
    print("res:{}".format(p.stdout.readlines()))

class GetHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        parsed_path = parse.urlparse(self.path)
        print("path data:",parsed_path.path)
        if parsed_path.path == "/node/stop":
            self.Stop();
            self.send_response_only(200)
        #self.send_header('Content-Type',
        #                 'text/plain; charset=utf-8')
        #self.end_headers()
        #self.wfile.write(bytes(json.dumps(js).encode('utf-8')))


    def do_POST(self):
        parsed_path = parse.urlparse(self.path)
        print("path data:",parsed_path.path)
        if parsed_path.path == "/node/deploy":
            content_len = int(self.headers['Content-Length'])
            post_body = str(self.rfile.read(content_len).decode())
            post_body=post_body.replace("%3A",":")
            data_list = post_body.split('&')
            addresses = []
            for data in data_list:
                if data.split("=")[0] == "address":
                    addresses.append(data.split("=")[1])
            self.StartServer(addresses)
            self.send_response_only(200)

    def GenerateServerConfig(self, addresses):
        config_data=ResConfigData() 
        config_data.self_region_id=1
        region = config_data.region.add()
        region.region_id = 1
        idx=1
        with open("{}/deploy/iplist.txt".format(script_path),"w") as f:
            for address in  addresses:
                (ip,port) = address.split(':')
                f.write("{}:{}\n".format(ip,port))
                replica = ReplicaInfo()
                replica.id = idx
                replica.ip = ip
                replica.port = int(port)
                region.replica_info.append(replica)
                idx+=1
        json_obj = MessageToJson(config_data)
        with open("{}/deploy/server.config".format(script_path),"w") as f:
            f.write(json_obj)
        return idx-2

    def Stop(self):
        RunCmd("{}/stop.sh".format(script_path))

    def Start(self, server_num):
        RunCmd("{}/generate_key.sh {}".format(script_path, script_path))
        RunCmd("{}/generate_config.sh {}".format(script_path, script_path))
        RunCmd("{}/start_kv_server.sh {} {}".format(script_path, script_path, server_num))

    def StartServer(self,addresses):
        server_num = self.GenerateServerConfig(addresses)
        self.Start(server_num)

if __name__ == '__main__':
    if len(sys.argv) == 1:
        print ("please provide the script path paramaters. Usage: bazel run :deploy_manager $PWD")
        exit(0)
    print(" working path:",sys.argv[1])
    script_path = sys.argv[1]
    from http.server import HTTPServer
    server = HTTPServer(('0.0.0.0', 4080), GetHandler)
    print('Starting server, use <Ctrl-C> to stop')
    server.serve_forever()
