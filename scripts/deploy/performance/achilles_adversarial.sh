#!/bin/bash
# Achilles adversarial performance test
# Usage: ./performance/achilles_adversarial.sh <ip_conf> <scenario_json> [phases_json]
export server=//benchmark/protocols/achilles:kv_server_performance
export TEMPLATE_PATH=$PWD/config/achilles.config
export ENABLE_TEE=1

./performance/adversarial_performance.sh $*
