#!/bin/bash
# ShoalPP performance test
export server=//benchmark/protocols/shoalpp:kv_server_performance
export TEMPLATE_PATH=$PWD/config/shoalpp.config

./performance/run_performance.sh $*
