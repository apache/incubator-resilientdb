#!/bin/sh

import sys

from oracle_script.comm.utils import *
from oracle_script.comm.comm_config import *

def gen_svr_config(config):
    iplist=get_ips(config["svr_ip_file"])
    with open(config["pow_config_path"],"w") as f:
        for idx,ip in iplist:
            port = int(config["base_port"]) + int(idx)
            f.writelines("{} {} {}\n".format(idx, ip, port))


def kill_svr(config):
    iplist=get_ips(config["svr_ip_file"])
    for (idx,svr_ip) in iplist:
        cmd="killall -9 {};".format(config["svr_bin"])
        run_remote_cmd(svr_ip, cmd)

def run_svr(config):
    iplist=get_ips(config["svr_ip_file"])
    for (idx,svr_ip) in iplist:
        private_key="{}/node_{}.key.pri".format("pow_cert",idx)
        cert="{}/cert_{}.cert".format("pow_cert",idx)
        cmd="nohup {} {} {} {} {} > pow_server{}.log 2>&1 &".format(
		    get_remote_file_name(config["svr_bin_bazel_path"]),
		    get_remote_file_name(config["bft_config_path"]), 
		    get_remote_file_name(config["pow_config_path"]), 
		    private_key, 
		    cert, idx)

        run_remote_cmd(svr_ip, cmd)


def upload_svr(config):
    iplist=get_ips(config["svr_ip_file"])
    for (idx,svr_ip) in iplist:
        run_remote_cmd(svr_ip, "rm -rf {}; rm -rf server.config; rm -rf cert; mkdir -p pow_cert;".format(
            get_remote_file_name(config["svr_bin_bazel_path"])))
        private_key=config["cert_path"]+"/"+ "node_"+idx+".key.pri"
        cert=config["cert_path"]+ "/"+ "cert_"+idx+".cert"
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["svr_bin_bazel_path"],svr_ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["bft_config_path"],svr_ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["pow_config_path"],svr_ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu/pow_cert".format(KEY,private_key, svr_ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu/pow_cert".format(KEY,cert, svr_ip))


if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    print("config:{}".format(config))
    gen_svr_config(config)
    kill_svr(config)
    upload_svr(config)
    run_svr(config)
