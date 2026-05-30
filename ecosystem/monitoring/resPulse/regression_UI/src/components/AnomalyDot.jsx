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

import { C } from "../constants";

export default function AnomalyDot({ cx, cy, payload, baseline, metricField, lowerBetter, onClick }) {
  if (!cx || !cy) return null;
  const bval = baseline?.[metricField]?.mean;
  let fill = C.accent;
  if (bval != null) {
    const worse = lowerBetter ? payload[metricField] > bval : payload[metricField] < bval;
    fill = worse ? C.red : C.green;
  }
  return <circle cx={cx} cy={cy} r={6} fill={fill} stroke={C.bg2} strokeWidth={2} style={{ cursor: "pointer" }} onClick={() => onClick && onClick(payload)} />;
}
