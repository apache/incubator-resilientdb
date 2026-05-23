const express   = require("express");
const router    = express.Router();
const scheduler = require("../scheduler");

// GET /api/schedule — returns the current scheduled interval and alert config
router.get("/", (req, res) => {
  const schedule = scheduler.getSchedule();
  res.json({
    success: true,
    interval: schedule.interval,
    alertConfig: schedule.alertConfig
  });
});

// POST /api/schedule — body: { interval: "onemin"|"hourly"|"daily"|"weekly"|"monthly", alertConfig?: {...} }
router.post("/", (req, res) => {
  const { interval, alertConfig } = req.body;
  if (!scheduler.INTERVALS[interval])
    return res.status(400).json({ success: false, error: `Invalid interval. Valid values: ${Object.keys(scheduler.INTERVALS).join(", ")}` });

  // Validate alertConfig if provided
  if (alertConfig && (!alertConfig.email || typeof alertConfig.thresholds !== 'object')) {
    return res.status(400).json({ success: false, error: "Invalid alertConfig. Must include email and thresholds object." });
  }

  const result = scheduler.setSchedule(interval, alertConfig);
  res.json({ success: true, interval: result.interval, alertConfig: result.alertConfig });
});

// DELETE /api/schedule — removes the scheduled run
router.delete("/", (req, res) => {
  scheduler.setSchedule(null);
  res.json({ success: true, interval: null, alertConfig: null });
});

module.exports = router;
