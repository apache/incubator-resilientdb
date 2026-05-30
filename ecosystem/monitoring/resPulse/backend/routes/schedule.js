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

const express   = require("express");
const router    = express.Router();
const scheduler = require("../scheduler");

// GET /api/schedule — returns the current scheduled interval, alert config, and baseline period
router.get("/", (req, res) => {
  const schedule = scheduler.getSchedule();
  res.json({
    success: true,
    interval: schedule.interval,
    alertConfig: schedule.alertConfig,
    baselinePeriod: schedule.baselinePeriod
  });
});

// POST /api/schedule — body: { interval: "hourly"|"daily"|"weekly"|"monthly", alertConfig?: {...}, baselinePeriod?: "1week"|"1month"|"3months"|"6months"|"1year" }
router.post("/", (req, res) => {
  const { interval, alertConfig, baselinePeriod } = req.body;
  if (!scheduler.INTERVALS[interval])
    return res.status(400).json({ success: false, error: `Invalid interval. Valid values: ${Object.keys(scheduler.INTERVALS).join(", ")}` });

  // Validate baselinePeriod if provided
  const validBaselinePeriods = ["1week", "1month", "3months", "6months", "1year"];
  if (baselinePeriod && !validBaselinePeriods.includes(baselinePeriod)) {
    return res.status(400).json({ success: false, error: `Invalid baseline period. Valid values: ${validBaselinePeriods.join(", ")}` });
  }

  // Validate alertConfig if provided
  if (alertConfig && (!alertConfig.email || typeof alertConfig.thresholds !== 'object')) {
    return res.status(400).json({ success: false, error: "Invalid alertConfig. Must include email and thresholds object." });
  }

  const result = scheduler.setSchedule(interval, alertConfig, baselinePeriod);
  res.json({ success: true, interval: result.interval, alertConfig: result.alertConfig, baselinePeriod: result.baselinePeriod });
});

// DELETE /api/schedule — removes the scheduled run
router.delete("/", (req, res) => {
  scheduler.setSchedule(null);
  res.json({ success: true, interval: null, alertConfig: null });
});

module.exports = router;
