#!/bin/bash
# Damysus adversarial performance test
# Usage: ./performance/damysus_adversarial.sh <ip_conf> <scenario_json> [phases_json]
export server=//benchmark/protocols/damysus:kv_server_performance
export TEMPLATE_PATH=$PWD/config/damysus.config
export ENABLE_TEE=1

./performance/adversarial_performance.sh $*
