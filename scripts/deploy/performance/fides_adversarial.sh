#!/bin/bash
# Fides adversarial performance test
# Usage: ./performance/fides_adversarial.sh <ip_conf> <scenario_json> [phases_json]
export server=//benchmark/protocols/fides:kv_server_performance
export TEMPLATE_PATH=$PWD/config/fides.config
export ENABLE_TEE=1

./performance/adversarial_performance.sh $*
