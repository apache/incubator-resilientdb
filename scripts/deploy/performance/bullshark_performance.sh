#!/bin/bash
# BullShark performance test
export server=//benchmark/protocols/bullshark:kv_server_performance
export TEMPLATE_PATH=$PWD/config/bullshark.config

./performance/run_performance.sh $*
