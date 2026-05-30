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

const PerfResult = require("./models/PerfResult");

const METRICS = [
  { id: "latency",    field: "avg_latency_ms",       lowerBetter: true  },
  { id: "consensus",  field: "consensus_ms_mean",    lowerBetter: true  },
  { id: "throughput", field: "throughput_rps",       lowerBetter: false },
  { id: "success",    field: "success_rate",         lowerBetter: false },
];

/**
 * Detects performance regressions by comparing the latest result to baseline
 * @param {Object} alertConfig - Configuration containing thresholds for each metric
 * @param {Object} latestResult - The most recent test result
 * @param {String} baselinePeriod - Period to use for baseline calculation ("1week"|"1month"|"3months"|"6months"|"1year")
 * @returns {Array} Array of detected regressions
 */
async function detectRegressions(alertConfig, latestResult, baselinePeriod = "6months") {
  console.log("[regression] detectRegressions called with:", {
    hasAlertConfig: !!alertConfig,
    thresholds: alertConfig?.thresholds,
    resultTimestamp: latestResult?.timestamp,
    baselinePeriod: baselinePeriod
  });

  if (!alertConfig || !alertConfig.thresholds) {
    console.log("[regression] No alert config or thresholds provided");
    return [];
  }

  try {
    // Get baseline data using the specified period
    const baseline = await calculateBaseline(latestResult.timestamp, baselinePeriod);
    if (!baseline) {
      console.log("[regression] No baseline data available for comparison");
      return [];
    }

    const regressions = [];

    // Check each metric that has a threshold configured
    for (const metric of METRICS) {
      const threshold = alertConfig.thresholds[metric.id];
      if (threshold == null || threshold <= 0) continue;

      const currentValue = getMetricValue(latestResult, metric);
      const baselineValue = getMetricValue(baseline, metric);

      if (currentValue == null || baselineValue == null) continue;

      const percentChange = ((currentValue - baselineValue) / baselineValue) * 100;
      const isRegression = metric.lowerBetter ?
        percentChange > threshold :
        percentChange < -threshold;

      if (isRegression) {
        regressions.push({
          metric: metric.id,
          metricLabel: getMetricLabel(metric.id),
          threshold: threshold,
          change: percentChange,
          currentValue,
          baselineValue,
          unit: getMetricUnit(metric.id)
        });
      }

      console.log(`[regression] ${metric.id}: ${percentChange.toFixed(1)}% change (threshold: ${threshold}%) ${isRegression ? '🚨' : '✓'}`);
    }

    console.log(`[regression] Detection complete. Found ${regressions.length} regressions:`, regressions.map(r => r.metric));
    return regressions;

  } catch (error) {
    console.error("[regression] Error detecting regressions:", error);
    return [];
  }
}

/**
 * Calculate baseline metrics from test results in the specified time period
 * @param {Date} beforeTimestamp - Only consider results before this timestamp
 * @param {String} baselinePeriod - Time period to consider for baseline
 */
async function calculateBaseline(beforeTimestamp, baselinePeriod = "6months") {
  try {
    // Convert baseline period to date range
    const cutoffDate = new Date(beforeTimestamp);
    switch(baselinePeriod) {
      case "1week":
        cutoffDate.setDate(cutoffDate.getDate() - 7);
        break;
      case "1month":
        cutoffDate.setMonth(cutoffDate.getMonth() - 1);
        break;
      case "3months":
        cutoffDate.setMonth(cutoffDate.getMonth() - 3);
        break;
      case "6months":
        cutoffDate.setMonth(cutoffDate.getMonth() - 6);
        break;
      case "1year":
        cutoffDate.setFullYear(cutoffDate.getFullYear() - 1);
        break;
      default:
        cutoffDate.setMonth(cutoffDate.getMonth() - 6); // Default to 6 months
    }

    const recentResults = await PerfResult
      .find({
        timestamp: {
          $lt: beforeTimestamp,
          $gte: cutoffDate
        }
      })
      .sort({ timestamp: -1 })
      .lean();

    console.log(`[regression] Found ${recentResults.length} results in ${baselinePeriod} baseline period`);

    if (recentResults.length < 3) {
      return null; // Need at least 3 results for a meaningful baseline
    }

    // Calculate averages
    const baseline = {};
    for (const metric of METRICS) {
      const values = recentResults
        .map(r => getMetricValue(r, metric))
        .filter(v => v != null);

      if (values.length > 0) {
        const fieldName = metric.field.replace('_mean', ''); // Handle consensus_ms_mean -> consensus_time_ms
        if (metric.field === 'consensus_ms_mean') {
          baseline.consensus_time_ms = { mean: values.reduce((a, b) => a + b) / values.length };
        } else {
          baseline[metric.field] = values.reduce((a, b) => a + b) / values.length;
        }
      }
    }

    return baseline;
  } catch (error) {
    console.error("[regression] Error calculating baseline:", error);
    return null;
  }
}

/**
 * Extract metric value from result object
 */
function getMetricValue(result, metric) {
  if (metric.field === 'consensus_ms_mean') {
    return result.consensus_time_ms?.mean;
  }
  return result[metric.field];
}

/**
 * Get human-readable metric label
 */
function getMetricLabel(metricId) {
  const labels = {
    latency: "Latency",
    consensus: "Consensus Time",
    throughput: "Throughput",
    success: "Success Rate"
  };
  return labels[metricId] || metricId;
}

/**
 * Get metric unit
 */
function getMetricUnit(metricId) {
  const units = {
    latency: "ms",
    consensus: "ms",
    throughput: "r/s",
    success: "%"
  };
  return units[metricId] || "";
}

module.exports = { detectRegressions };