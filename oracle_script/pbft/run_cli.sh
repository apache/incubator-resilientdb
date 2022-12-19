#!/bin/sh

CONFIG_PATH=$1
bazel build //benchmark/pbft:benchmark_client_main
bazel run //oracle_script/pbft/script:run_cli $CONFIG_PATH
