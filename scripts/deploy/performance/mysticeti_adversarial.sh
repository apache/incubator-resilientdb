#!/bin/bash
# Mysticeti adversarial performance test
export server=//benchmark/protocols/mysticeti:kv_server_performance
export TEMPLATE_PATH=$PWD/config/mysticeti.config

./performance/adversarial_performance.sh $*
