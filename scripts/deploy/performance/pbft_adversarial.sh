#!/bin/bash
# PBFT adversarial performance test
export server=//benchmark/protocols/pbft:kv_server_performance
export TEMPLATE_PATH=$PWD/config/pbft.config

./performance/adversarial_performance.sh $*
