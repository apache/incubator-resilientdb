#!/bin/bash
# ShoalPP adversarial performance test
export server=//benchmark/protocols/shoalpp:kv_server_performance
export TEMPLATE_PATH=$PWD/config/shoalpp.config

./performance/adversarial_performance.sh $*
