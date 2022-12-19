#!/bin/sh

import sys

from oracle_script.comm.utils import *
from oracle_script.comm.comm_config import *

def check_ready(config):
    iplist=get_ips(config["svr_ip_file"])
    for (idx,svr_ip) in iplist:
        cmd="grep \'is ready\' /home/ubuntu/server{}.log | grep Server | tail -1".format(idx)
        res=run_remote_cmd(svr_ip, cmd)
        print("svr:{} res:{}".format(svr_ip, res))

if __name__ == '__main__':
    config_file=sys.argv[1]
    config = read_config(config_file)
    check_ready(config)

