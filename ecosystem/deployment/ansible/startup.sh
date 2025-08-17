#!/bin/bash
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

# Wait a few seconds to let systemd fully boot
sleep 5

echo "Reloading systemd daemon..."
systemctl daemon-reload

echo "Enabling and starting all custom services..."
systemctl enable nginx && systemctl start nginx
systemctl enable crow-http && systemctl start crow-http
systemctl enable graphql && systemctl start graphql
systemctl enable resilientdb-client && systemctl start resilientdb-client
systemctl enable resilientdb-kv@1 && systemctl start resilientdb-kv@1
systemctl enable resilientdb-kv@2 && systemctl start resilientdb-kv@2
systemctl enable resilientdb-kv@3 && systemctl start resilientdb-kv@3
systemctl enable resilientdb-kv@4 && systemctl start resilientdb-kv@4

echo "Custom services started."
