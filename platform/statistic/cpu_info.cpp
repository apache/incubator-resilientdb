/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "platform/statistic/cpu_info.h"

#include <glog/logging.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>
#include <sys/sysinfo.h>
#include <string.h>

namespace resdb {

namespace {

void get_cpuoccupy (cpu_occupy_t *cpust)
{
    FILE *fd;
    char buff[256];
    cpu_occupy_t *cpu_occupy;
    cpu_occupy=cpust;

    fd = fopen ("/proc/stat", "r");
    assert(fd != nullptr);

    fgets (buff, sizeof(buff), fd);

    sscanf (buff, "%s %u %u %u %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle ,&cpu_occupy->iowait,&cpu_occupy->irq,&cpu_occupy->softirq);

    LOG(ERROR)<<" name:"<< cpu_occupy->name<<" user:"<<cpu_occupy->user<<" nice:"<<cpu_occupy->nice<<" sys:"<<cpu_occupy->system<<" idle:"<<cpu_occupy->idle <<" iowait:"<<cpu_occupy->iowait<<" irq:"<<cpu_occupy->irq<<" soft irq:"<<cpu_occupy->softirq;

    fclose(fd);
}

double cal_cpuoccupy (cpu_occupy_t *o, cpu_occupy_t *n)
{
    double od, nd;
    double id, sd;
    double cpu_use ;

    od = (double) (o->user + o->nice + o->system +o->idle+o->softirq+o->iowait+o->irq);
    nd = (double) (n->user + n->nice + n->system +n->idle+n->softirq+n->iowait+n->irq);

    id = (double) (n->idle);    
    sd = (double) (o->idle) ;  
    if((nd-od) != 0)
        cpu_use =100.0 - ((id-sd))/(nd-od)*100.00; 
    else
        cpu_use = 0;
    return cpu_use;
}


}

CPUInfo::CPUInfo() {
  pid_ = getpid();
  LOG(ERROR)<<" get pid:"<<pid_;
}

double CPUInfo::GetCPUUsage(){
  cpu_occupy_t current_cpu_stat;
  get_cpuoccupy(&current_cpu_stat);
  double cpu = cal_cpuoccupy (&cpu_stat_, &current_cpu_stat);
  cpu_stat_ = current_cpu_stat;
  return cpu;
}

}
