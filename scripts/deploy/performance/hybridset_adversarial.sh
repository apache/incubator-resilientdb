#!/bin/bash
# HybridSet adversarial performance test
export server=//benchmark/protocols/hybridset:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hybridset.config
export ENABLE_TEE=1

./performance/adversarial_performance.sh $*
