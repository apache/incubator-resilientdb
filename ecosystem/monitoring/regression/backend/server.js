require("dotenv").config();
const express  = require("express");
const cors     = require("cors");
const mongoose = require("mongoose");
const { spawn } = require("child_process");
const path     = require("path");

const resultsRouter = require("./routes/results");

const app  = express();
const PORT = process.env.PORT || 5000;

app.use(cors({
  origin: ["http://localhost:5173", "http://localhost:3000", "http://localhost:5000"],
  methods: ["GET", "POST", "DELETE"],
  allowedHeaders: ["Content-Type"],
}));
app.use(express.json());

app.use("/api/results", resultsRouter);

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
  child.on("close", (code) => {
    if (res.headersSent) return;
    if (code === 0) {
      res.json({ success: true, message: "Test completed", output: stderr });
    } else {
      res.status(500).json({ success: false, error: `Test exited with code ${code}`, output: stderr });
    }
  });
});

app.get("/api/health", (req, res) => {
  res.json({ status: "ok", db: mongoose.connection.readyState === 1 ? "connected" : "disconnected" });
});

mongoose
  .connect(process.env.MONGO_URI)
  .then(() => {
    console.log("✓ Connected to MongoDB");
    app.listen(PORT, () => console.log(`✓ Server running on http://localhost:${PORT}`));
  })
  .catch((err) => {
    console.error("✗ MongoDB connection failed:", err.message);
    process.exit(1);
  });
