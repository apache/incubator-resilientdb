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

// POST /api/schedule — body: { interval: "onemin"|"hourly"|"daily"|"weekly"|"monthly", alertConfig?: {...}, baselinePeriod?: "1week"|"1month"|"3months"|"6months"|"1year" }
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
