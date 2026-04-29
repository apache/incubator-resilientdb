#!/bin/bash
# Mysticeti performance test
export server=//benchmark/protocols/mysticeti:kv_server_performance
export TEMPLATE_PATH=$PWD/config/mysticeti.config

./performance/run_performance.sh $*
