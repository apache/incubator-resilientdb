require("dotenv").config();
const express  = require("express");
const cors     = require("cors");
const mongoose = require("mongoose");
const { spawn } = require("child_process");
const path     = require("path");

const resultsRouter   = require("./routes/results");
const scheduleRouter  = require("./routes/schedule");
const scheduler       = require("./scheduler");
const { detectRegressions } = require("./regressionDetector");
const { sendRegressionAlert } = require("./alerting");
const PerfResult = require("./models/PerfResult");

const app  = express();
const PORT = process.env.PORT || 8000;

app.use(cors({
  origin: ["http://localhost:5173", "http://localhost:3000", "http://localhost:8000"],
  methods: ["GET", "POST", "DELETE"],
  allowedHeaders: ["Content-Type"],
}));
app.use(express.json());

app.use("/api/results",  resultsRouter);
app.use("/api/schedule", scheduleRouter);

app.post("/api/run-test", (req, res) => {
  const scriptPath = path.join(__dirname, "..", "perf_test.sh");
  const runs       = parseInt(req.body?.runs) || 500;
  const version    = req.body?.version || "";

  const child = spawn("bash", [scriptPath, String(runs), version], {
    cwd: path.join(__dirname, ".."),
  });

  let stderr = "";
  child.stderr.on("data", (data) => { stderr += data.toString(); });
  child.on("error", (err) => {
    if (!res.headersSent) res.status(500).json({ success: false, error: err.message });
  });
  child.on("close", async (code) => {
    if (res.headersSent) return;
    if (code === 0) {
      res.json({ success: true, message: "Test completed", output: stderr });

      // Check for regressions if alerting is configured
      const schedule = scheduler.getSchedule();
      if (schedule.alertConfig && schedule.alertConfig.email) {
        setTimeout(async () => {
          try {
            console.log("[manual-test] Checking for regressions...");
            const latestResult = await PerfResult.findOne().sort({ timestamp: -1 }).lean();
            if (latestResult) {
              const regressions = await detectRegressions(schedule.alertConfig, latestResult);
              if (regressions.length > 0) {
                console.log(`[manual-test] ${regressions.length} regression(s) detected, sending alert...`);
                await sendRegressionAlert(schedule.alertConfig, regressions, latestResult);
              }
            }
          } catch (error) {
            console.error("[manual-test] Error checking for regressions:", error);
          }
        }, 2000);
      }
    } else {
      res.status(500).json({ success: false, error: `Test exited with code ${code}`, output: stderr });
    }
  });
});

app.get("/api/health", (req, res) => {
  res.json({ status: "ok", db: mongoose.connection.readyState === 1 ? "connected" : "disconnected" });
});

mongoose
  .connect(process.env.MONGO_URI || process.env.MONGODB_URL)
  .then(() => {
    console.log("✓ Connected to MongoDB");
    scheduler.init();
    app.listen(PORT, () => console.log(`✓ Server running on http://localhost:${PORT}`));
  })
  .catch((err) => {
    console.error("✗ MongoDB connection failed:", err.message);
    process.exit(1);
  });
