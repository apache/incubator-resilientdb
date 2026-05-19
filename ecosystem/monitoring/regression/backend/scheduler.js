const cron       = require("node-cron");
const fs         = require("fs");
const path       = require("path");
const { spawn }  = require("child_process");

const STATE_FILE  = path.join(__dirname, "schedule.json");
const SCRIPT_PATH = path.resolve(__dirname, "../perf_test.sh");

const INTERVALS = {
  onemin:  "*/1 * * * *",
  hourly:  "0 * * * *",
  daily:   "0 0 * * *",
  weekly:  "0 0 * * 0",
  monthly: "0 0 1 * *",
};

let activeTask     = null;
let activeInterval = null;

function runTest() {
  console.log(`[scheduler] running perf_test.sh at ${new Date().toISOString()}`);
  const child = spawn("bash", [SCRIPT_PATH], {
    cwd:      path.dirname(SCRIPT_PATH),
    stdio:    "ignore",
    detached: true,
  });
  child.unref();
}

function loadState() {
  try {
    const data = JSON.parse(fs.readFileSync(STATE_FILE, "utf-8"));
    return INTERVALS[data.interval] ? data.interval : null;
  } catch { return null; }
}

function saveState(interval) {
  fs.writeFileSync(STATE_FILE, JSON.stringify({ interval: interval ?? null }), "utf-8");
}

function setSchedule(interval) {
  if (activeTask) { activeTask.stop(); activeTask = null; }

  if (!interval || !INTERVALS[interval]) {
    activeInterval = null;
    saveState(null);
    return null;
  }

  activeTask     = cron.schedule(INTERVALS[interval], runTest);
  activeInterval = interval;
  saveState(interval);
  console.log(`[scheduler] scheduled: ${interval} (${INTERVALS[interval]})`);
  return interval;
}

function getSchedule() {
  return activeInterval;
}

function init() {
  const saved = loadState();
  if (saved) {
    setSchedule(saved);
    console.log(`[scheduler] restored schedule: ${saved}`);
  }
}

module.exports = { init, getSchedule, setSchedule, INTERVALS };
