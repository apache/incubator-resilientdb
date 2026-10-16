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
# End-to-end test for secondary indexes (composite keys), issue #180.
# Starts a local cluster with LevelDB, runs every index command against it and
# checks the results. Run from the repository root:
#
#   ./service/tools/kv/api_tools/composite_key_e2e_test.sh
#
# Exits 0 and prints PASS if everything matched, non-zero otherwise.

set -u

if [ ! -f WORKSPACE ]; then
  echo "run this from the repository root"
  exit 1
fi

# Leave no servers behind, even on Ctrl-C.
trap 'killall -9 kv_service 2>/dev/null' EXIT INT TERM

CONFIG=service/tools/config/interface/service.config
CLI=bazel-bin/service/tools/kv/api_tools/kv_service_tools
CERT_DIR=service/tools/data/cert
FAILURES=0

run() {
  $CLI --config $CONFIG "$@" 2>/dev/null
}

# Prints just the primary keys a query returned, sorted, space separated.
# A request that never reached the servers must not look like "no matches",
# so the header line has to be there before we strip it.
query() {
  local index=$1
  local attrs=${2:-}
  local out
  if [ -z "$attrs" ]; then
    out=$(run --cmd query_by_index --index "$index")
  else
    out=$(run --cmd query_by_index --index "$index" --attrs "$attrs")
  fi
  case "$out" in
    *"result(s)"*) ;;
    *)
      echo "QUERY FAILED"
      return
      ;;
  esac
  echo "$out" | tail -n +2 | sort | tr '\n' ' '
}

check() {
  local what=$1 got=$2 want=$3
  if [ "$got" = "$want" ]; then
    echo "  ok   $what"
  else
    echo "  FAIL $what"
    echo "       expected: '$want'"
    echo "       got:      '$got'"
    FAILURES=$((FAILURES + 1))
  fi
}

start_cluster() {
  killall -9 kv_service 2>/dev/null
  sleep 1
  ./service/tools/kv/server_tools/start_kv_service.sh \
      --define enable_leveldb=True > /dev/null 2>&1
  sleep 5
}

echo "== setup =="
if [ ! -f "$CERT_DIR/cert_1.cert" ]; then
  echo "  generating keys and certificates"
  ./service/tools/kv/server_tools/generate_keys_and_certs.sh > /dev/null 2>&1
fi
bazel build //service/tools/kv/api_tools:kv_service_tools > /dev/null 2>&1 ||
  { echo "  FAIL could not build the CLI"; exit 1; }
rm -rf ./*_db
start_cluster
echo "  cluster started with a fresh database"

echo "== store records =="
# set is fire-and-forget, so every write is confirmed with a read.
run --cmd set --key photo:1 --value jpeg-1 > /dev/null
run --cmd set --key photo:2 --value png-2 > /dev/null
run --cmd set --key photo:3 --value jpeg-3 > /dev/null
check "photo:1 stored" "$(run --cmd get --key photo:1)" \
      "get key = photo:1 value = jpeg-1"
check "photo:2 stored" "$(run --cmd get --key photo:2)" \
      "get key = photo:2 value = png-2"
check "photo:3 stored" "$(run --cmd get --key photo:3)" \
      "get key = photo:3 value = jpeg-3"

echo "== create index entries =="
run --cmd create_index_entry --index by_owner_format --attrs shray,jpeg \
    --pk photo:1 > /dev/null
run --cmd create_index_entry --index by_owner_format --attrs shray,png \
    --pk photo:2 > /dev/null
run --cmd create_index_entry --index by_owner_format --attrs ana,jpeg \
    --pk photo:3 > /dev/null
check "query by owner"        "$(query by_owner_format shray)" "photo:1 photo:2 "
check "query by owner+format" "$(query by_owner_format shray,jpeg)" "photo:1 "
check "whole index"           "$(query by_owner_format)" "photo:1 photo:2 photo:3 "
check "no match"              "$(query by_owner_format nobody)" ""
# Attributes match from the left only: "jpeg" is the second one.
check "second attribute alone finds nothing" \
      "$(query by_owner_format jpeg)" ""

echo "== rejected requests =="
check "indexing a record that does not exist" \
      "$(run --cmd create_index_entry --index by_owner_format --attrs x,y \
             --pk photo:999 | grep -c 'ret = -3')" "1"
check "deleting an entry that does not exist" \
      "$(run --cmd delete_index_entry --index by_owner_format --attrs x,y \
             --pk photo:1 | grep -c 'ret = -3')" "1"
check "updating an entry that does not exist" \
      "$(run --cmd update_index_entry --index by_owner_format --attrs x,y \
             --new_attrs a,b --pk photo:1 | grep -c 'ret = -3')" "1"
check "rejected create left the index unchanged" \
      "$(query by_owner_format)" "photo:1 photo:2 photo:3 "

echo "== update =="
run --cmd update_index_entry --index by_owner_format --attrs shray,png \
    --new_attrs shray,jpeg --pk photo:2 > /dev/null
check "moved out of the old attributes" "$(query by_owner_format shray,png)" ""
check "moved into the new attributes" \
      "$(query by_owner_format shray,jpeg)" "photo:1 photo:2 "

echo "== delete =="
run --cmd delete_index_entry --index by_owner_format --attrs ana,jpeg \
    --pk photo:3 > /dev/null
check "entry removed" "$(query by_owner_format ana)" ""
check "the record itself is untouched" "$(run --cmd get --key photo:3)" \
      "get key = photo:3 value = jpeg-3"

echo "== index entries stay out of normal reads =="
check "get on a record is unaffected" "$(run --cmd get --key photo:1)" \
      "get key = photo:1 value = jpeg-1"
# "ck\0..." sorts between the photo keys, and an index entry's value is empty,
# so an unfiltered range scan would splice empty elements into this list.
check "range scan hides index entries" \
      "$(run --cmd get_key_range --min_key ' ' --max_key '~' | tr -d '\000' |
         grep -acE '\[,|,,|,\]')" "0"

echo "== restart: the index is on disk =="
start_cluster
check "index survived the restart" \
      "$(query by_owner_format shray,jpeg)" "photo:1 photo:2 "
check "records survived the restart" "$(run --cmd get --key photo:1)" \
      "get key = photo:1 value = jpeg-1"

echo "== teardown =="
killall -9 kv_service 2>/dev/null
echo "  cluster stopped"

if [ $FAILURES -eq 0 ]; then
  echo "PASS"
  exit 0
fi
echo "FAIL: $FAILURES check(s) failed"
exit 1
