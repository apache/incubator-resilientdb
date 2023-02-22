SERVER_PATH=./bazel-bin/sdk_client/crow_service_main
KV_CLIENT_CONFIG=example/kv_client_config.config
CHAIN_SERVER_CONFIG=sdk_client/server_config.config

killall -9 crow_service_main

bazel build //sdk_client:crow_service_main

$SERVER_PATH $KV_CLIENT_CONFIG $CHAIN_SERVER_CONFIG