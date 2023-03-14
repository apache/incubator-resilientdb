#!/usr/bin/python

import os,sys,datetime,re
import shlex
import subprocess
from hostnames import *

machines=hostip
cmd = './set_ulimit.sh \"{}\"'.format(' '.join(machines))
print(cmd)
os.system(cmd)
