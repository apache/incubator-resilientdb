<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

# Performance Metrics Explanation

This document explains how `analyze.py` converts raw `curl` timing output from `perf_test.sh` into benchmark metrics, historical comparisons, diagnosis messages, and dashboard-ready JSON.

The goal is to make the performance analyzer easier to understand for future developers and for project documentation.

---

## 1. Where the Data Comes From

The benchmark script sends transaction requests directly to the ResilientDB API endpoint. For each request, `curl` returns timing information in this format:

```text
HTTP_CODE time_connect time_pretransfer time_starttransfer time_total
```

Example:

```text
200 0.00010 0.00015 0.00048 0.00053
```

These timing values are reported by `curl` in **seconds**. The analyzer multiplies them by `1000` to convert them into **milliseconds**.

Using the example above:

```text
time_connect       = 0.00010 sec  -> 0.10 ms
time_pretransfer   = 0.00015 sec  -> 0.15 ms
time_starttransfer = 0.00048 sec  -> 0.48 ms
time_total         = 0.00053 sec  -> 0.53 ms
```

---

## 2. Main Curl Timing Values

### 2.1 Total Latency

**Source:**

```text
time_total
```

**Meaning:**

Total latency is the full end-to-end request time. It measures the time from when the request starts until the full response is received.

**Formula:**

```text
total_latency_ms = time_total * 1000
```

**Example:**

```text
0.00053 sec * 1000 = 0.53 ms
```

This is the main latency value shown in the UI.

---

### 2.2 Average Latency

**Meaning:**

Average latency is the average end-to-end request time across all parsed requests.

**Formula:**

```text
avg_latency_ms = sum(total_latency_ms values) / number_of_requests
```

**Example:**

```text
latencies = [0.40, 0.50, 0.60]
avg_latency_ms = (0.40 + 0.50 + 0.60) / 3
avg_latency_ms = 0.50 ms
```

If the UI says:

```text
Avg Latency = 0.50 ms
```

that means the average full request time was `0.50 ms`.

---

### 2.3 TCP Connect Time

**Source:**

```text
time_connect
```

**Meaning:**

TCP connect time measures how long it took to establish the TCP connection.

**Formula:**

```text
tcp_connect_ms = time_connect * 1000
```

**Example:**

```text
0.00010 sec * 1000 = 0.10 ms
```

**Interpretation:**

If this value is high, the benchmark may be affected by network setup overhead, socket exhaustion, connection reuse issues, or network delay. If this value is low, the slowdown is probably not caused by basic connection setup.

---

### 2.4 Pre-Transfer Time

**Source:**

```text
time_pretransfer
```

**Meaning:**

Pre-transfer time measures the time until `curl` is ready to begin transferring the request. For plain HTTP, this is usually close to connection setup time. For HTTPS, it may also include TLS handshake time.

**Formula:**

```text
pretransfer_ms = time_pretransfer * 1000
```

**Example:**

```text
0.00015 sec * 1000 = 0.15 ms
```

This value is usually not displayed directly in the UI, but it is used to calculate server-side wait time.

---

### 2.5 Start-Transfer Time

**Source:**

```text
time_starttransfer
```

**Meaning:**

Start-transfer time measures the time from the start of the request until the first byte of the server response is received.

**Formula:**

```text
starttransfer_ms = time_starttransfer * 1000
```

**Example:**

```text
0.00048 sec * 1000 = 0.48 ms
```

This value is also not usually displayed directly in the UI, but it is used to calculate server-side wait time and transfer time.

---

## 3. Derived Timing Values

### 3.1 Server-Side Wait Time

**Formula:**

```text
server_wait_ms = time_starttransfer_ms - time_pretransfer_ms
```

**Example:**

```text
starttransfer_ms = 0.48 ms
pretransfer_ms   = 0.15 ms
server_wait_ms   = 0.48 - 0.15
server_wait_ms   = 0.33 ms
```

**Meaning:**

Server-side wait time estimates how long the client waited after the request was ready to send and before the server began sending its response.

This may include:

- PBFT processing
- request queuing
- replica coordination
- application execution
- response preparation
- server scheduling delay

**Important note:**

This value is **not guaranteed to be pure PBFT consensus time**. It is safer to describe it as a client-observed proxy for server-side delay. If the UI displays this as consensus-related, it should make clear that this is an indirect measurement.

A safer label is:

```text
Server-Side Wait Time
```

rather than:

```text
Consensus Time
```

---

### 3.2 Transfer Time

**Formula:**

```text
transfer_ms = total_latency_ms - time_starttransfer_ms
```

**Example:**

```text
total_latency_ms = 0.53 ms
starttransfer_ms = 0.48 ms
transfer_ms      = 0.53 - 0.48
transfer_ms      = 0.05 ms
```

**Meaning:**

Transfer time measures how long it took to receive the rest of the response after the first byte arrived.

**Interpretation:**

If this value is high, the issue may involve:

- large response payloads
- slow response transfer
- network return-path delay
- client-side receiving overhead

---

### 3.3 Throughput

**Formula:**

```text
throughput_rps = number_of_completed_requests / total_elapsed_seconds
```

In the current analyzer:

```text
total_elapsed_seconds = sum(total_latency_ms values) / 1000
```

**Example:**

```text
completed requests = 500
sum of request latencies = 252.36 ms

total_elapsed_seconds = 252.36 / 1000
total_elapsed_seconds = 0.25236 sec

throughput_rps = 500 / 0.25236
throughput_rps = 1981.28 requests/sec
```

**Important note:**

If `perf_test.sh` sends requests sequentially, this value should be interpreted as **sequential client throughput**, not true multi-client concurrent throughput.

---

### 3.4 Success Rate

**Formula:**

```text
success_rate = successful_requests / total_attempted_requests * 100
```

**Example:**

```text
successful requests = 500
total attempted requests = 500

success_rate = 500 / 500 * 100
success_rate = 100%
```

**Meaning:**

Success rate shows the percentage of attempted transactions that returned a successful response.

---

## 4. Statistical Summary Values

The analyzer computes summary statistics for several timing groups:

- total latency
- server-side wait time
- TCP connect time
- transfer time

Each group gets a summary containing:

```text
mean, min, max, standard deviation, p50, p95, p99
```

---

### 4.1 p50 Latency

**Meaning:**

p50 is the median request latency.

**Interpretation:**

If:

```text
p50 = 0.36 ms
```

then 50% of requests completed at or below `0.36 ms`.

This represents the typical request.

---

### 4.2 p95 Latency

**Meaning:**

p95 is the 95th percentile request latency.

**Interpretation:**

If:

```text
p95 = 0.89 ms
```

then 95% of requests completed at or below `0.89 ms`, while the slowest 5% took longer.

This helps identify slower-than-normal requests that may be hidden by average latency.

---

### 4.3 p99 Latency

**Meaning:**

p99 is the 99th percentile request latency.

**Interpretation:**

If:

```text
p99 = 2.30 ms
```

then 99% of requests completed at or below `2.30 ms`, while the slowest 1% were outliers.

This helps detect rare but important tail-latency spikes.

---

### 4.4 Standard Deviation

**Meaning:**

Standard deviation measures how spread out the request latencies are.

**Interpretation:**

Low standard deviation means requests are completing consistently.

Example:

```text
0.49 ms, 0.50 ms, 0.51 ms
```

High standard deviation means some requests are much slower or faster than the average.

Example:

```text
0.30 ms, 0.35 ms, 2.30 ms
```

High standard deviation can suggest unstable behavior, intermittent replica lag, uneven batching, scheduling noise, or temporary server stalls.

---

## 5. Historical Baseline Comparison

The analyzer fetches historical baseline metrics from the backend/MongoDB and compares the current run against previous stored runs.

The percentage change formula is:

```text
percent_change = ((current_value - baseline_value) / baseline_value) * 100
```

---

### 5.1 Latency Comparison Example

```text
current latency  = 0.50 ms
baseline latency = 0.38 ms
```

```text
percent_change = ((0.50 - 0.38) / 0.38) * 100
percent_change = 31.6%
```

This means latency increased by about `31.6%`.

A positive latency change usually means performance got worse because requests are taking longer.

---

### 5.2 Throughput Comparison Example

```text
current throughput  = 1981.28 req/s
baseline throughput = 8965.05 req/s
```

```text
percent_change = ((1981.28 - 8965.05) / 8965.05) * 100
percent_change = -77.9%
```

This means throughput decreased by `77.9%`.

A negative throughput change usually means performance got worse because the system is processing fewer requests per second.

---

### 5.3 Server-Side Wait Comparison

Server-side wait time is also compared against the historical baseline.

If server-side wait increases significantly, it means the client is waiting longer before the server begins responding.

This may point to additional delay in:

- PBFT processing
- request queuing
- replica coordination
- commit execution
- response preparation

---

## 6. Pattern-Based Analysis

After computing the current metrics and baseline differences, the analyzer applies heuristic pattern rules to generate readable diagnosis messages and recommendations.

The analysis checks for patterns such as:

- tail-latency amplification
- high server-side wait share
- elevated TCP connection overhead
- transfer time exceeding server-side wait
- high latency variance
- low or unusually high throughput
- latency regression against baseline
- throughput regression against baseline
- server-side wait regression against baseline

These rules are meant to provide a first-pass explanation of system behavior. They do not prove the root cause, but they help guide the next debugging step.

---

## 7. Overall Status

The analyzer assigns an overall status based on how many warning signals are present.

Possible statuses include:

```text
stable
minor_warning
needs_attention
possible_regression
```

Examples of severity signals include:

- latency increased more than 25% compared to baseline
- throughput dropped more than 25% compared to baseline
- standard deviation is greater than average latency
- success rate is below 100%
- p99 latency is much higher than p50 latency

The more signals present, the more severe the final status becomes.

---

## 8. Full Analysis Flow

The analyzer follows this pipeline:

```text
1. Receive raw curl timing output from perf_test.sh.
2. Split the raw string into individual request records.
3. Extract HTTP code, connect time, pre-transfer time, start-transfer time, and total time.
4. Convert all timing values from seconds to milliseconds.
5. Calculate derived metrics:
   - average latency
   - throughput
   - success rate
   - server-side wait time
   - TCP connect time
   - transfer time
6. Calculate statistical summaries:
   - mean
   - min
   - max
   - standard deviation
   - p50
   - p95
   - p99
7. Fetch historical baseline metrics from MongoDB through the backend API.
8. Compare the current run against the historical baseline.
9. Generate diagnosis messages and recommendations using pattern-based rules.
10. Save the result back to MongoDB.
11. Print final JSON for the dashboard/frontend.
```

## 9. Important Limitations

### Curl timing is client-observed

The analyzer measures what the client sees. It does not directly instrument the internal PBFT phases.

### Server-side wait is not pure consensus time

`time_starttransfer - time_pretransfer` is a useful proxy for server-side delay, but it may include more than consensus. It can also include queuing, execution, serialization, and scheduling delay.

### Sequential throughput is not true concurrent throughput

If the benchmark sends requests one at a time, throughput should be described as sequential client throughput. True scalability testing requires concurrent clients or parallel request generation.

### Pattern-based recommendations are heuristic

The diagnosis and recommendations are meant to guide debugging, not prove root cause. Internal logs, Prometheus metrics, ResLens profiling, or PBFT phase traces should be used to validate the explanation.

