const cron       = require("node-cron");
const fs         = require("fs");
const path       = require("path");
const { spawn }  = require("child_process");
const { detectRegressions } = require("./regressionDetector");
const { sendRegressionAlert } = require("./alerting");
const PerfResult = require("./models/PerfResult");

const STATE_FILE  = path.join(__dirname, "schedule.json");
const SCRIPT_PATH = path.resolve(__dirname, "../perf_test.sh");

const INTERVALS = {
  hourly:  "0 * * * *",
  daily:   "0 0 * * *",
  weekly:  "0 0 * * 0",
  monthly: "0 0 1 * *",
};

let activeTask      = null;
let activeInterval  = null;
let alertConfig     = null;
let baselinePeriod  = null;

function runTest() {
  console.log(`[scheduler] running perf_test.sh at ${new Date().toISOString()}`);
  const child = spawn("bash", [SCRIPT_PATH], {
    cwd:      path.dirname(SCRIPT_PATH),
    stdio:    "ignore",
    detached: true,
  });

  // If alerting is configured, check for regressions after test completes
  if (alertConfig && alertConfig.email) {
    child.on("close", async (code) => {
      if (code === 0) {
        console.log("[scheduler] Test completed, checking for regressions...");
        await checkForRegressions();
      }
    });
  }

  child.unref();
}

async function checkForRegressions() {
  try {
    // Wait a moment for the result to be saved to the database
    await new Promise(resolve => setTimeout(resolve, 2000));

    // Get the latest result
    const latestResult = await PerfResult.findOne().sort({ timestamp: -1 }).lean();
    if (!latestResult) {
      console.log("[scheduler] No results found for regression check");
      return;
    }

    // Detect regressions
    const regressions = await detectRegressions(alertConfig, latestResult, baselinePeriod);

    if (regressions.length > 0) {
      console.log(`[scheduler] ${regressions.length} regression(s) detected, sending alert...`);
      await sendRegressionAlert(alertConfig, regressions, latestResult);
    } else {
      console.log("[scheduler] No regressions detected");
    }

  } catch (error) {
    console.error("[scheduler] Error checking for regressions:", error);
  }
}

function loadState() {
  try {
    const data = JSON.parse(fs.readFileSync(STATE_FILE, "utf-8"));
    return {
      interval: INTERVALS[data.interval] ? data.interval : null,
      alertConfig: data.alertConfig || null,
      baselinePeriod: data.baselinePeriod || "6months"
    };
  } catch { return { interval: null, alertConfig: null, baselinePeriod: "6months" }; }
}

function saveState(interval, alertCfg = null, basePeriod = null) {
  fs.writeFileSync(STATE_FILE, JSON.stringify({
    interval: interval ?? null,
    alertConfig: alertCfg ?? alertConfig,
    baselinePeriod: basePeriod ?? baselinePeriod
  }), "utf-8");
}

function setSchedule(interval, alertCfg = null, basePeriod = null) {
  if (activeTask) { activeTask.stop(); activeTask = null; }

  if (!interval || !INTERVALS[interval]) {
    activeInterval = null;
    alertConfig = null;
    baselinePeriod = null;
    saveState(null, null, null);
    return { interval: null, alertConfig: null, baselinePeriod: null };
  }

  activeTask     = cron.schedule(INTERVALS[interval], runTest);
  activeInterval = interval;
  if (alertCfg !== null) alertConfig = alertCfg;
  if (basePeriod !== null) baselinePeriod = basePeriod;
  else if (baselinePeriod === null) baselinePeriod = "6months"; // Set default
  saveState(interval, alertConfig, baselinePeriod);
  console.log(`[scheduler] scheduled: ${interval} (${INTERVALS[interval]}) with baseline: ${baselinePeriod}`);
  return { interval, alertConfig, baselinePeriod };
}

function getSchedule() {
  return { interval: activeInterval, alertConfig, baselinePeriod };
}

function init() {
  const saved = loadState();
  if (saved.interval) {
    alertConfig = saved.alertConfig;
    baselinePeriod = saved.baselinePeriod;
    setSchedule(saved.interval, saved.alertConfig, saved.baselinePeriod);
    console.log(`[scheduler] restored schedule: ${saved.interval} with baseline: ${saved.baselinePeriod}`);
  }
}

module.exports = { init, getSchedule, setSchedule, INTERVALS };
