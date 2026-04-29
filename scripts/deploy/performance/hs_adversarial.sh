#!/bin/bash
# HotStuff adversarial performance test
export server=//benchmark/protocols/hs:kv_server_performance
export TEMPLATE_PATH=$PWD/config/hs.config

./performance/adversarial_performance.sh $*
