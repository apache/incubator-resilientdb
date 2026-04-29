#!/bin/bash
# RCC adversarial performance test
export server=//benchmark/protocols/rcc:kv_server_performance
export TEMPLATE_PATH=$PWD/config/rcc.config

./performance/adversarial_performance.sh $*
