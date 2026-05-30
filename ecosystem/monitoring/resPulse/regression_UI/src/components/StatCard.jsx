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

export default function StatCard({ label, value, delta, anomaly }) {
  return (
    <div style={{ background: C.bg2, border: `1px solid ${anomaly ? "rgba(255,71,87,0.35)" : C.border}`, borderRadius: 8, padding: "1rem 1.2rem", position: "relative", overflow: "hidden" }}>
      <div style={{ position: "absolute", top: 0, left: 0, right: 0, height: 2, background: anomaly ? C.red : C.accent, opacity: anomaly ? 1 : 0.4 }} />
      <div style={{ fontSize: 10, color: C.text3, letterSpacing: 2, textTransform: "uppercase", marginBottom: 6 }}>{label}</div>
      <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1, marginBottom: 4 }}>{value}</div>
      <div style={{ fontSize: 11 }}>{delta}</div>
    </div>
  );
}
