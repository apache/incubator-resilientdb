#!/bin/bash

bazel-bin/service/http_server/crow_service_main service/tools/config/interface/service.config service/http_server/server_config.config
tail -f /dev/null
