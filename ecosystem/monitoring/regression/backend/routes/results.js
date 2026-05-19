const express    = require("express");
const router     = express.Router();
const path       = require("path");
const { spawnSync } = require("child_process");
const PerfResult = require("../models/PerfResult");

// GET /
// Retrieves a list of performance test results with optional pagination and date range filtering.
// Query parameters: limit (default 200), from (start date), to (end date)
// Returns results sorted by timestamp in descending order.
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

// GET /baseline
// Computes and returns baseline statistics from the last 6 months of performance data.
// Calculates mean, standard deviation, and count for latency, throughput, success rate, and consensus time.
// Used by the analyzer to detect performance regressions against historical trends.
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

// POST /
// Creates and stores a new performance test result in the database.
// Required fields: timestamp, avg_latency_ms, throughput_rps
// Called by the analyze.py script after each performance test run.
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

// POST /:id/analyze
// On-demand analysis of an existing result against the baseline for a given time window.
// Body: { hours: number | null, periodLabel: string }
// Returns analysis JSON without saving to the database.
router.post("/:id/analyze", async (req, res) => {
  try {
    const record = await PerfResult.findById(req.params.id).lean();
    if (!record) return res.status(404).json({ success: false, error: "Record not found" });

    const { hours = null, periodLabel = "" } = req.body;

    const tsFilter = { $lt: new Date(record.timestamp) };
    if (hours != null) tsFilter.$gte = new Date(Date.now() - hours * 60 * 60 * 1000);

    const results = await PerfResult.find({ _id: { $ne: record._id }, timestamp: tsFilter }).lean();

    let baseline = null;
    if (results.length) {
      function avg(arr) { return arr.reduce((a, b) => a + b, 0) / arr.length; }
      function stddev(arr, mean) { return Math.sqrt(arr.reduce((a, b) => a + (b - mean) ** 2, 0) / arr.length); }
      baseline = {
        count:        results.length,
        period_start: hours != null ? new Date(Date.now() - hours * 60 * 60 * 1000) : null,
      };
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
    }

    const scriptPath = path.join(__dirname, "..", "analyze_record.py");
    const input      = JSON.stringify({ record, baseline, period_label: periodLabel });
    const proc       = spawnSync("python3", [scriptPath], { input, encoding: "utf-8", timeout: 10000 });

    if (proc.error)   throw proc.error;
    if (proc.status !== 0) throw new Error(proc.stderr || "Analysis script failed");

    res.json({ success: true, data: JSON.parse(proc.stdout) });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

// DELETE /:id
// Deletes a performance result record by its MongoDB document ID.
// Used for removing erroneous or invalid test results from the database.
router.delete("/:id", async (req, res) => {
  try {
    await PerfResult.findByIdAndDelete(req.params.id);
    res.json({ success: true, message: "Deleted" });
  } catch (err) {
    res.status(500).json({ success: false, error: err.message });
  }
});

module.exports = router;
