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
#!/bin/bash
# =============================================================================
# perf_test.sh
# ResilientDB Performance Benchmark
#
# Usage:
#   bash perf_test.sh [RUNS] [VERSION_TAG]
#
# Examples:
#   bash perf_test.sh 500
#   bash perf_test.sh 500 "abc1234 - My commit message"
# =============================================================================

# ENDPOINT="127.0.0.1:18000/v1/transactions/commit"
ENDPOINT="https://dev-crow.resilientdb.com/v1/transactions/commit"
RUNS=${1:-100}
VERSION=${2:-""}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SUCCESS=0
FAILED=0
RAW_TIMES=""

echo "Running $RUNS requests..." >&2

for i in $(seq 1 $RUNS); do
echo "Running $i of $RUNS requests..." >&2
  RESULT=$(curl -s -o /dev/null \
    -w "%{http_code} %{time_connect} %{time_pretransfer} %{time_starttransfer} %{time_total}" \
    -X POST \
    -H "Content-Type: application/json" \
    -d "{\"id\":\"perfkey$i\",\"value\":\"perfval$i\"}" \
    "$ENDPOINT")
    

  HTTP_CODE=$(echo "$RESULT" | awk '{print $1}')
  if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "201" ]; then
    SUCCESS=$((SUCCESS + 1))
  else
    FAILED=$((FAILED + 1))
  fi

  RAW_TIMES="$RAW_TIMES|$RESULT"
done

echo "Done. Running analysis... " >&2

# Pass raw data to analyze.py
python3 "$SCRIPT_DIR/analyze.py" \
  --runs "$RUNS" \
  --success "$SUCCESS" \
  --failed "$FAILED" \
  --version "$VERSION" \
  --raw "$RAW_TIMES"