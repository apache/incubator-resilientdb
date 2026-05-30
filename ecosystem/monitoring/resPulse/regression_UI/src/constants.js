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

export const API_URL = "";

export const METRICS = [
  { id: "latency",    label: "Latency",    unit: "ms",  field: "avg_latency_ms",    lowerBetter: true  },
  { id: "consensus",  label: "Consensus",  unit: "ms",  field: "consensus_ms_mean", lowerBetter: true  },
  { id: "throughput", label: "Throughput", unit: "r/s", field: "throughput_rps",    lowerBetter: false },
  { id: "success",    label: "Success",    unit: "%",   field: "success_rate",       lowerBetter: false },
];

export const C = {
  bg:      "#0a0c0f",
  bg2:     "#111318",
  bg3:     "#181c23",
  border:  "#1e2430",
  border2: "#252d3a",
  text:    "#e2e8f0",
  text2:   "#7a8aa0",
  text3:   "#4a5568",
  accent:  "#00d4ff",
  green:   "#00e676",
  red:     "#ff4757",
  orange:  "#ffa726",
  purple:  "#b388ff",
};
