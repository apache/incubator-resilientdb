/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*/

const mongoose = require("mongoose");

const PercentileSchema = new mongoose.Schema({
  mean: Number, min: Number, max: Number,
  stddev: Number, p50: Number, p95: Number, p99: Number,
}, { _id: false });

const PerfResultSchema = new mongoose.Schema({
  timestamp:         { type: Date, required: true },
  runs:              { type: Number, required: true },
  success_rate:      { type: Number, required: true },
  throughput_rps:    { type: Number, required: true },
  avg_latency_ms:    { type: Number, required: true },
  total_latency:     PercentileSchema,
  consensus_time_ms: PercentileSchema,
  tcp_connect_ms:    PercentileSchema,
  transfer_time_ms:  PercentileSchema,
  analysis:          { type: String },
  version:           { type: String },
}, { timestamps: true });

PerfResultSchema.index({ timestamp: -1 });

module.exports = mongoose.model("PerfResult", PerfResultSchema);
