const express = require("express");
const router  = express.Router();
const PerfResult = require("../models/PerfResult");

router.get("/", async (req, res) => {
  try {
    const { limit = 200, from, to } = req.query;
    const filter = {};
    if (from || to) {
      filter.timestamp = {};
      if (from) filter.timestamp.$gte = new Date(from);
      if (to)   filter.timestamp.$lte = new Date(to);
    }
    const results = await PerfResult.find(filter).sort({ timestamp: -1 }).limit(parseInt(limit)).lean();
    res.json({ success: true, count: results.length, data: results });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

router.get("/baseline", async (req, res) => {
  try {
    const sixMonthsAgo = new Date();
    sixMonthsAgo.setMonth(sixMonthsAgo.getMonth() - 6);
    const results = await PerfResult.find({ timestamp: { $gte: sixMonthsAgo } }).lean();
    if (!results.length) return res.json({ success: true, data: null, message: "Not enough data for baseline" });
    function avg(arr) { return arr.reduce((a, b) => a + b, 0) / arr.length; }
    function stddev(arr, mean) { return Math.sqrt(arr.reduce((a, b) => a + (b - mean) ** 2, 0) / arr.length); }
    const baseline = {};
    ["avg_latency_ms", "throughput_rps", "success_rate"].forEach((f) => {
      const vals = results.map((r) => r[f]).filter((v) => v != null);
      if (!vals.length) return;
      const mean = avg(vals);
      baseline[f] = { mean, stddev: stddev(vals, mean), count: vals.length };
    });
    const cvals = results.map((r) => r.consensus_time_ms?.mean).filter((v) => v != null);
    if (cvals.length) {
      const mean = avg(cvals);
      baseline.consensus_time_ms = { mean, stddev: stddev(cvals, mean), count: cvals.length };
    }
    res.json({ success: true, data: baseline, period_start: sixMonthsAgo, count: results.length });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

router.post("/", async (req, res) => {
  try {
    const body = req.body;
    if (!body.timestamp || body.avg_latency_ms == null || body.throughput_rps == null)
      return res.status(400).json({ success: false, error: "Missing required fields" });
    const result = await PerfResult.create(body);
    res.status(201).json({ success: true, data: result });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

router.delete("/:id", async (req, res) => {
  try {
    await PerfResult.findByIdAndDelete(req.params.id);
    res.json({ success: true, message: "Deleted" });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

module.exports = router;
