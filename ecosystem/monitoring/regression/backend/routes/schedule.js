const express   = require("express");
const router    = express.Router();
const scheduler = require("../scheduler");

// GET /api/schedule — returns the current scheduled interval or null
router.get("/", (req, res) => {
  res.json({ success: true, interval: scheduler.getSchedule() });
});

// POST /api/schedule — body: { interval: "onemin"|"hourly"|"daily"|"weekly"|"monthly" }
router.post("/", (req, res) => {
  const { interval } = req.body;
  if (!scheduler.INTERVALS[interval])
    return res.status(400).json({ success: false, error: `Invalid interval. Valid values: ${Object.keys(scheduler.INTERVALS).join(", ")}` });

  const result = scheduler.setSchedule(interval);
  res.json({ success: true, interval: result });
});

// DELETE /api/schedule — removes the scheduled run
router.delete("/", (req, res) => {
  scheduler.setSchedule(null);
  res.json({ success: true, interval: null });
});

module.exports = router;
