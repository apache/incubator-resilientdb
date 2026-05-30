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

import { METRICS, C } from "./constants";

export function flattenRecord(r) {
  return { ...r, consensus_ms_mean: r.consensus_time_ms?.mean ?? null };
}

export function detectAnomalies(record, baseline) {
  if (!baseline) return [];
  const flat = flattenRecord(record);
  return METRICS
    .map(({ field, lowerBetter }) => {
      const bval = baseline[field]?.mean;
      const val  = flat[field];
      if (bval == null || val == null) return null;
      const diff = val - bval;
      if (diff === 0) return null;
      return { field, diff, pct: (diff / bval) * 100, worse: lowerBetter ? diff > 0 : diff < 0 };
    })
    .filter(Boolean);
}

export function statusColor(status) {
  switch (status) {
    case "possible_regression": return C.red;
    case "needs_attention":     return C.orange;
    case "minor_warning":       return C.orange;
    default:                    return C.green;
  }
}

export function statusLabel(status) {
  switch (status) {
    case "possible_regression": return "⚠ POSSIBLE REGRESSION";
    case "needs_attention":     return "⚠ NEEDS ATTENTION";
    case "minor_warning":       return "⚡ MINOR WARNING";
    default:                    return "✓ STABLE";
  }
}
