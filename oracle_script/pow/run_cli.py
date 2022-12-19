#!/bin/sh

import sys
from oracle_script.comm.utils import *
from oracle_script.comm.comm_config import *

cert_path="pow_cert"

def upload_cli(config):
    iplist=get_ips(config["cli_ip_file"])
    for (idx,ip) in iplist:
        run_remote_cmd(ip, "rm -rf {};".format(get_remote_file_name(config["cli_start_script"])))
        run_remote_cmd(ip, "rm -rf {};".format(get_remote_file_name(config["cli_bin_bazel_path"])))
        run_remote_cmd(ip, "rm -rf server.config; rm -rf cert; mkdir -p pow_cert;")

        private_key=config["cert_path"]+"/"+ "node_"+idx+".key.pri"
        cert=config["cert_path"]+ "/"+ "cert_"+idx+".cert"

        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["cli_bin_bazel_path"],ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["bft_config_path"],ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu/{}/".format(KEY,private_key, ip, cert_path))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu/{}/".format(KEY,cert, ip, cert_path))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY, config["cli_start_script"], ip))

def run_cli(config):
    iplist=get_ips(config["cli_ip_file"])
    for (idx,ip) in iplist:
        private_key="{}/node_{}.key.pri".format(cert_path,idx)
        cert="{}/cert_{}.cert".format(cert_path,idx)

        cmd="sh {} {} {} {} {} {}".format(
                    get_remote_file_name(config["cli_start_script"]),
                    get_remote_file_name(config["cli_bin_bazel_path"]),
		    get_remote_file_name(config["bft_config_path"]),
		    private_key, 
		    cert, ip)
        res=run_remote_cmd(ip, cmd)
        for line in res:
            print (str(line.strip().decode("utf-8") ))

if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    print("config:{}".format(config))
    upload_cli(config)
    run_cli(config)
