#!/bin/bash
# BullShark adversarial performance test
export server=//benchmark/protocols/bullshark:kv_server_performance
export TEMPLATE_PATH=$PWD/config/bullshark.config

./performance/adversarial_performance.sh $*
