#!/bin/sh

import sys
import os
from oracle_script.comm.utils import *
from oracle_script.comm.comm_config import *

cert_path="pbft_cert"

def gen_cli_config(config):
    iplist=get_ips(config["cli_ip_file"])
    with open(config["cli_config_path"],"w") as f:
        for idx,ip in iplist:
            port = int(config["base_port"]) + int(idx)
            f.writelines("{} {} {}\n".format(idx, ip, port))

def upload_cli(config):
    iplist=get_ips(config["cli_ip_file"])

    for (idx,ip) in iplist:
        run_remote_cmd(ip, "rm -rf {}; rm -rf {}, {};".format(
                get_remote_file_name(config["cli_start_script"]), 
                get_remote_file_name(config["cli_config_path"]),
                get_remote_file_name(config["cli_bin_bazel_path"])))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["cli_config_path"],ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY,config["cli_bin_bazel_path"],ip))
        run_cmd_with_resp("scp -i {} {} ubuntu@{}:/home/ubuntu".format(KEY, config["cli_start_script"], ip))

def run_cli(config):
    iplist=get_ips(config["cli_ip_file"])
    cmds=[]
    for (idx,ip) in iplist:
        cmd="sh {} {} {}".format(
                    get_remote_file_name(config["cli_start_script"]),
                    get_remote_file_name(config["cli_bin_bazel_path"]),
		    get_remote_file_name(config["cli_config_path"])
		    )
        cmds.append((ip,cmd))

    run_remote_cmd_list(cmds)

if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    print("config:{}".format(config))
    gen_cli_config(config)
    upload_cli(config)
    run_cli(config)
