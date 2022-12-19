#!/bin/sh

import sys

from oracle_script.comm.utils import *
from oracle_script.comm.comm_config import *

cert_path="pbft_cert/"

def gen_svr_config(config):
    iplist=get_ips(config["svr_ip_file"])
    with open(config["svr_config_path"],"w") as f:
        for idx,ip in iplist:
            port = int(config["base_port"]) + int(idx)
            f.writelines("{} {} {}\n".format(idx, ip, port))


def kill_svr(config):
    iplist=get_ips(config["svr_ip_file"])+get_ips(config["cli_ip_file"])
    cmd_list=[]
    for (idx,svr_ip) in iplist:
        cmd="killall -9 {};".format(config["svr_bin"])
        cmd_list.append((svr_ip,cmd))
        run_remote_cmd(svr_ip, cmd)
    #run_remote_cmd_list(cmd_list)

def run_svr(config):
    iplist=get_ips(config["svr_ip_file"])
    cli_iplist=get_ips(config["cli_ip_file"])
    for (idx,svr_ip) in iplist+cli_iplist:
        private_key="{}/node_{}.key.pri".format(cert_path,idx)
        cert="{}/cert_{}.cert".format(cert_path,idx)
        if [idx,svr_ip] in iplist:
            cmd="nohup {} {} {} {} > server{}.log 2>&1 &".format(
		    get_remote_file_name(config["svr_bin_bazel_path"]),
		    get_remote_file_name(config["svr_config_path"]), 
		    private_key, 
		    cert, idx)
        else:
            cmd="nohup {} {} {} {} > client{}.log 2>&1 &".format(
		    get_remote_file_name(config["svr_bin_bazel_path"]),
		    get_remote_file_name(config["svr_config_path"]), 
		    private_key, 
		    cert, idx)

        run_remote_cmd(svr_ip, cmd)

def upload_svr(config):
    iplist=get_ips(config["svr_ip_file"]) + get_ips(config["cli_ip_file"])
    cmd_list=[]
    for (idx,svr_ip) in iplist:
        run_remote_cmd(svr_ip, "rm -rf {}; rm server*.log; rm -rf server.config; rm -rf cert; mkdir -p {};".format(get_remote_file_name(config["svr_bin_bazel_path"]), cert_path))
        private_key=config["cert_path"]+"/"+ "node_"+idx+".key.pri"
        cert=config["cert_path"]+ "/"+ "cert_"+idx+".cert"

        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["svr_bin_bazel_path"],svr_ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["svr_config_path"],svr_ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu/{}".format(KEY,private_key, svr_ip, cert_path))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu/{}".format(KEY,cert, svr_ip, cert_path))
    #run_remote_cmd_list_raw(cmd_list)


if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    print("config:{}".format(config))
    #gen_svr_config(config)
    kill_svr(config)
    upload_svr(config)
    run_svr(config)
