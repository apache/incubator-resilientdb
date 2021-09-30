#!/usr/bin/python
#
# Command line arguments:
# [1] -- Number of server nodes
# [2] -- Name of result file

import os
import sys
import datetime
import re
import shlex
import subprocess
import itertools
from sys import argv
from hostnames import *
import socket

dashboard = None
home_directory = "/home/ubuntu"
PATH = os.getcwd()
#result_dir = PATH + "/results/"
result_dir = home_directory+"/resilientdb/results/"

# Total nodes
nds = int(argv[1])
resfile = argv[2]
run = int(argv[3])
send_ifconfig = True

#cmd = "mkdir -p {}".format(result_dir)
# os.system(cmd)
cmd = "cp config.h {}".format(result_dir)
os.system(cmd)


machines = hostip
mach = hostmach

#	# check all rundb/runcl are killed
cmd = './vcloud_cmd.sh \"{}\" \"pkill -f \'rundb\'\"'.format(' '.join(machines))
print(cmd)
os.system(cmd)
cmd = './vcloud_cmd.sh \"{}\" \"pkill -f \'runcl\'\"'.format(' '.join(machines))
print(cmd)
# cmd = './vcloud_cmd.sh \"{}\" \"mkdir -p \'resilientdb\'\"'.format(' '.join(machines))
# print(cmd)
os.system(cmd)

# if run == 0:
os.system("./scp_binaries.sh {} {}".format(nds, 1 if send_ifconfig else 0))

if dashboard is not None:
    hostname = socket.gethostname()
    IPAddr = socket.gethostbyname(hostname)
    print("Influx IP Address is:", IPAddr)

    # check if monitorResults.sh processes are killed
    os.system(
        './vcloud_cmd.sh \"{}\" \"pkill -f \'sh monitorResults.sh\'\"'.format(' '.join(machines)))

    # running monitorResults
    cmd_monitor = './vcloud_monitor.sh \"{}\" \"{}\"'.format(
        ' '.join(machines), IPAddr)
    print(cmd_monitor)
    os.system(cmd_monitor)

# running the experiment
cmd = './vcloud_deploy.sh \"{}\" {} \"{}\"'.format(' '.join(machines), nds, resfile)
print(cmd)
os.system(cmd)

if dashboard is not None:
    # check if monitorResults.sh processes are killed
    os.system(
        './vcloud_cmd.sh \"{}\" \"pkill -f \'sh monitorResults.sh\'\"'.format(' '.join(machines)))

# collecting the output
os.system("./scp_results.sh {} {} {}".format(nds, resfile, result_dir))
os.system("cp config.h {}".format(result_dir + "config_" + resfile))
