/**
 * ResilientDBMonitor.jsx
 *
 * Main performance monitoring dashboard component for Apache ResilientDB.
 * Displays real-time and historical performance metrics for the PBFT consensus protocol,
 * including latency, throughput, and success rate. Features include:
 * - Live connection status and performance metrics via stat cards
 * - Interactive line charts showing performance trends over time
 * - Time range selector (24h, 1 week, 1 month, 3 months, 6 months, all time)
 * - Anomaly detection highlighting unusual performance values
 * - Baseline comparison for detecting performance regressions
 * - Run history with detailed information overlay
 * - Auto-refresh with manual refresh capability
 * - Dark-themed UI with grid background and monospace font
 */

import { useState, useEffect, useRef, useCallback } from "react";
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip, ReferenceLine, ResponsiveContainer,
} from "recharts";

import { API_URL, METRICS, C } from "./constants";
import { flattenRecord, statusColor, statusLabel, detectAnomalies } from "./utils";
import StatCard from "./components/StatCard";
import Spinner from "./components/Spinner";
import AnomalyDot from "./components/AnomalyDot";
import DetailOverlay from "./components/DetailOverlay";
import ScheduleModal from "./components/ScheduleModal";

const TIME_RANGES = [
  { id: "24h", label: "24h",      hours: 24 },
  { id: "1w",  label: "1 Week",   hours: 24 * 7 },
  { id: "1m",  label: "1 Month",  hours: 24 * 30 },
  { id: "3m",  label: "3 Months", hours: 24 * 90 },
  { id: "6m",  label: "6 Months", hours: 24 * 180 },
  { id: "all", label: "All Time", hours: null },
];

export default function ResilientDBMonitor() {
  const [records,        setRecords]        = useState([]);
  const [apiBaseline,    setApiBaseline]    = useState(null);
  const [loading,        setLoading]        = useState(true);
  const [error,          setError]          = useState(null);
  const [activeMetric,   setActiveMetric]   = useState("latency");
  const [selected,       setSelected]       = useState(null);
  const [lastRefresh,    setLastRefresh]    = useState(null);
  const [testRunning,    setTestRunning]    = useState(false);
  const [notification,   setNotification]   = useState(null);
  const [timeRange,      setTimeRange]      = useState("6m");
  const [dropdownOpen,   setDropdownOpen]   = useState(false);
  const [schedule,       setSchedule]       = useState(null);
  const [scheduleInput,  setScheduleInput]  = useState("off");
  const [scheduleSaving, setScheduleSaving] = useState(false);
  const [alertConfig,    setAlertConfig]    = useState(null);
  const [alertEmail,     setAlertEmail]     = useState("");
  const [alertThresholds, setAlertThresholds] = useState([{ metric: "latency", threshold: 10 }]);
  const [scheduleModalOpen, setScheduleModalOpen] = useState(false);
  const rangeInitialized = useRef(false);
  const dropdownRef      = useRef(null);

  const fetchData = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const [resultsRes, baselineRes, scheduleRes] = await Promise.all([
        fetch(`${API_URL}/api/results?limit=200`),
        fetch(`${API_URL}/api/results/baseline`),
        fetch(`${API_URL}/api/schedule`),
      ]);
      const resultsJson  = await resultsRes.json();
      const baselineJson = await baselineRes.json();
      const scheduleJson = await scheduleRes.json();
      if (resultsJson.success)                        setRecords(resultsJson.data);
      if (baselineJson.success && baselineJson.data)  setApiBaseline(baselineJson.data);
      if (scheduleJson.success) {
        setSchedule(scheduleJson.interval ?? null);
        setScheduleInput(scheduleJson.interval ?? "off");
        setAlertConfig(scheduleJson.alertConfig ?? null);
        if (scheduleJson.alertConfig) {
          setAlertEmail(scheduleJson.alertConfig.email || "");
          // Convert old object format to new array format
          const thresholds = scheduleJson.alertConfig.thresholds || {};
          const thresholdArray = Object.entries(thresholds).map(([metric, threshold]) => ({
            metric,
            threshold
          }));
          setAlertThresholds(thresholdArray.length > 0 ? thresholdArray : [{ metric: "latency", threshold: 10 }]);
        }
      }
      setLastRefresh(new Date());
    } catch {
      setError(`Could not connect to backend at ${API_URL}. Make sure the server is running.`);
    } finally {
      setLoading(false);
    }
  }, []);

  // Check if alert configuration has changed
  function hasAlertConfigChanged() {
    if (!alertEmail.trim()) return alertConfig !== null;

    // Convert array back to object format for comparison
    const thresholdObj = alertThresholds.reduce((acc, { metric, threshold }) => {
      acc[metric] = threshold;
      return acc;
    }, {});

    const currentConfig = { email: alertEmail.trim(), thresholds: thresholdObj };
    return JSON.stringify(currentConfig) !== JSON.stringify(alertConfig);
  }

  async function saveSchedule() {
    setScheduleSaving(true);
    try {
      // Prepare alert config if email is provided
      const alertConfigPayload = alertEmail.trim()
        ? {
            email: alertEmail.trim(),
            thresholds: alertThresholds.reduce((acc, { metric, threshold }) => {
              acc[metric] = threshold;
              return acc;
            }, {})
          }
        : null;

      const res = scheduleInput === "off"
        ? await fetch(`${API_URL}/api/schedule`, { method: "DELETE" })
        : await fetch(`${API_URL}/api/schedule`, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
              interval: scheduleInput,
              alertConfig: alertConfigPayload
            }),
          });
      const j = await res.json();
      if (j.success) {
        setSchedule(j.interval ?? null);
        setScheduleInput(j.interval ?? "off");
        setAlertConfig(j.alertConfig ?? null);
        setScheduleModalOpen(false);
      }
    } finally {
      setScheduleSaving(false);
    }
  }

  async function deleteSchedule() {
    setScheduleSaving(true);
    try {
      const res = await fetch(`${API_URL}/api/schedule`, { method: "DELETE" });
      const j = await res.json();
      if (j.success) {
        // Clear all related state
        setSchedule(null);
        setScheduleInput("off");
        setAlertConfig(null);
        setAlertEmail("");
        setAlertThresholds([{ metric: "latency", threshold: 10 }]);
        setScheduleModalOpen(false);
        console.log("Schedule deleted successfully");
      }
    } catch (error) {
      console.error("Failed to delete schedule:", error);
    } finally {
      setScheduleSaving(false);
    }
  }

  async function runTest() {
    setTestRunning(true);
    setNotification("running");
    try {
      const res  = await fetch(`${API_URL}/api/run-test`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({}) });
      const json = await res.json();
      if (json.success) {
        setNotification("completed");
        fetchData();
      } else {
        setNotification("error");
      }
    } catch {
      setNotification("error");
    } finally {
      setTestRunning(false);
      setTimeout(() => setNotification(null), 4000);
    }
  }

  useEffect(() => { fetchData(); }, [fetchData]);

  // While a schedule is active, silently poll for new results every 20 s
  useEffect(() => {
    if (!schedule) return;
    const id = setInterval(async () => {
      try {
        const res = await fetch(`${API_URL}/api/results?limit=200`);
        const j   = await res.json();
        if (j.success) {
          setRecords(j.data);
          setLastRefresh(new Date());
        }
      } catch {}
    }, 20000);
    return () => clearInterval(id);
  }, [schedule]);

  // Show "Chart Updated" notification whenever the time range changes (skip initial mount)
  useEffect(() => {
    if (!rangeInitialized.current) { rangeInitialized.current = true; return; }
    setNotification("chart_updated");
    setTimeout(() => setNotification(null), 3000);
  }, [timeRange]);

  // Close custom dropdown when clicking outside
  useEffect(() => {
    if (!dropdownOpen) return;
    function handleOutside(e) {
      if (dropdownRef.current && !dropdownRef.current.contains(e.target)) setDropdownOpen(false);
    }
    document.addEventListener("mousedown", handleOutside);
    return () => document.removeEventListener("mousedown", handleOutside);
  }, [dropdownOpen]);

  const sorted = [...records].sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));

  const filteredSorted = (() => {
    const range = TIME_RANGES.find(r => r.id === timeRange);
    if (!range || !range.hours) return sorted;
    const cutoff = new Date(Date.now() - range.hours * 60 * 60 * 1000);
    return sorted.filter(r => new Date(r.timestamp) >= cutoff);
  })();

  // Fall back to the global latest if the filtered window is empty
  const latest = filteredSorted[filteredSorted.length - 1] ?? sorted[sorted.length - 1] ?? null;
  const metric = METRICS.find(m => m.id === activeMetric);
  const activeRange = TIME_RANGES.find(r => r.id === timeRange);

  const localBaseline = (() => {
    // Use the API baseline only when "All Time" is selected and the API returned one
    if (timeRange === "all" && apiBaseline) return apiBaseline;
    if (filteredSorted.length < 2) return null;
    const bl = {};
    METRICS.forEach(({ field }) => {
      const vals = filteredSorted.map(r => flattenRecord(r)[field]).filter(v => v != null);
      if (!vals.length) return;
      const mean   = vals.reduce((a, b) => a + b, 0) / vals.length;
      const stddev = Math.sqrt(vals.reduce((a, b) => a + (b - mean) ** 2, 0) / vals.length);
      bl[field] = { mean, stddev };
    });
    return bl;
  })();

  const chartData   = filteredSorted.map(r => ({ ...flattenRecord(r), _label: new Date(r.timestamp).toLocaleDateString("en-US", { month: "short", day: "numeric" }), _original: r }));
  const baselineVal = localBaseline?.[metric.field]?.mean;

  function statValue(m) {
    if (!latest) return "—";
    const val = flattenRecord(latest)[m.field];
    if (val == null) return "—";
    return `${val.toFixed(m.id === "throughput" ? 2 : 1)} ${m.unit}`;
  }

  function statDelta(m) {
    if (!localBaseline || !latest) return <span style={{ color: C.text3 }}>no baseline</span>;
    const bval = localBaseline[m.field]?.mean;
    const val  = flattenRecord(latest)[m.field];
    if (bval == null || val == null) return <span style={{ color: C.text3 }}>—</span>;
    const diff  = val - bval;
    const pct   = ((diff / bval) * 100).toFixed(1);
    const worse = m.lowerBetter ? diff > 0 : diff < 0;
    return <span style={{ color: worse ? C.red : C.green }}>{diff > 0 ? "↑" : "↓"} {diff > 0 ? "+" : ""}{pct}% vs baseline</span>;
  }

  function statAnomalous(m) {
    if (!localBaseline || !latest) return false;
    const bval = localBaseline[m.field]?.mean;
    const val  = flattenRecord(latest)[m.field];
    if (bval == null || val == null) return false;
    return m.lowerBetter ? val > bval : val < bval;
  }

  function CustomTooltip({ active, payload }) {
    if (!active || !payload?.length) return null;
    const d = payload[0].payload;
    return (
      <div style={{ background: C.bg2, border: `1px solid ${C.border}`, borderRadius: 6, padding: "8px 12px", fontSize: 11 }}>
        <div style={{ color: C.accent, marginBottom: 4 }}>{d._label}</div>
        <div style={{ color: C.text2 }}>{metric.label}: <span style={{ color: C.text }}>{d[metric.field]?.toFixed(2)} {metric.unit}</span></div>
        {baselineVal != null && <div style={{ color: C.text3 }}>Baseline: {baselineVal.toFixed(2)} {metric.unit}</div>}
      </div>
    );
  }

  const notifColor = (notification === "completed" || notification === "deleted")
    ? C.green
    : notification === "error"
      ? C.red
      : C.accent;

  return (
    <div style={{ background: C.bg, minHeight: "100vh", fontFamily: "'JetBrains Mono', monospace", color: C.text, position: "relative", overflowX: "hidden" }}>
      <div style={{ position: "fixed", inset: 0, backgroundImage: `linear-gradient(rgba(0,212,255,0.03) 1px, transparent 1px), linear-gradient(90deg, rgba(0,212,255,0.03) 1px, transparent 1px)`, backgroundSize: "40px 40px", pointerEvents: "none", zIndex: 0 }} />

      <div style={{ position: "relative", zIndex: 1, maxWidth: 1200, margin: "0 auto", padding: "2rem 1.5rem" }}>

        {/* Header */}
        <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", marginBottom: "2.5rem", paddingBottom: "1.5rem", borderBottom: `1px solid ${C.border}` }}>
          <div style={{ display: "flex", alignItems: "center", gap: 14 }}>
            <img
              src="/images/simple_Logo.png"
              alt="ResPulse Logo"
              style={{
                width: 110,
                height: 110,
                objectFit: "contain",
                filter: "drop-shadow(0 0 8px rgba(0, 212, 255, 0.25))"
              }}
            />
            <div style={{ display: "flex", flexDirection: "column", gap: 5 }}>
              <span style={{ fontSize: 10, color: C.accent, letterSpacing: 3, textTransform: "uppercase", fontWeight: 500 }}>PBFT Performance Monitoring</span>
              <span style={{
                fontFamily: "'Syne', sans-serif",
                fontSize: 28,
                fontWeight: 800,
                letterSpacing: -0.8,
                background: `linear-gradient(135deg, ${C.accent}, #00f5ff, #0099ff)`,
                backgroundClip: "text",
                WebkitBackgroundClip: "text",
                color: "transparent",
                textShadow: "none",
                filter: "drop-shadow(0 0 1px rgba(0, 212, 255, 0.3))"
              }}>ResPulse</span>
              <span style={{ fontSize: 11, color: "#8da3b0", letterSpacing: 1 }}>Performance Regression Detection and Explanation</span>
              </div>
          </div>
          <div style={{ textAlign: "right", display: "flex", flexDirection: "column", alignItems: "flex-end", gap: 8 }}>
            <div style={{ display: "flex", alignItems: "center", gap: 8, fontSize: 11, color: C.text2 }}>
              <span style={{ width: 6, height: 6, borderRadius: "50%", background: error ? C.red : C.green, boxShadow: `0 0 8px ${error ? C.red : C.green}`, display: "inline-block" }} />
              {error ? "DISCONNECTED" : "LIVE"}
            </div>
            <div style={{ fontSize: 11, color: C.text3 }}>{records.length} run{records.length !== 1 ? "s" : ""} in database</div>
            {lastRefresh && <div style={{ fontSize: 10, color: C.text3 }}>Last refresh: {lastRefresh.toLocaleTimeString()}</div>}
            <div style={{ display: "flex", gap: 8 }}>
              <button onClick={fetchData} disabled={loading}
                style={{ padding: "6px 14px", fontSize: 10, letterSpacing: 1, textTransform: "uppercase", fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, border: `1px solid ${C.border2}`, borderRadius: 5, cursor: loading ? "not-allowed" : "pointer", background: "transparent", color: loading ? C.text3 : C.accent }}>
                {loading ? "Loading..." : "↻ Refresh"}
              </button>
              <button onClick={runTest} disabled={testRunning || loading}
                style={{ padding: "6px 14px", fontSize: 10, letterSpacing: 1, textTransform: "uppercase", fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, border: `1px solid ${testRunning ? C.border2 : C.green}`, borderRadius: 5, cursor: (testRunning || loading) ? "not-allowed" : "pointer", background: testRunning ? "transparent" : "rgba(0,230,118,0.08)", color: testRunning ? C.text3 : C.green }}>
                {testRunning ? "Running..." : "▶ Run Test"}
              </button>
            </div>
            {/* Schedule row */}
            <div style={{ display: "flex", alignItems: "center", gap: 8, justifyContent: "flex-end" }}>
              <button onClick={() => setScheduleModalOpen(true)}
                style={{ padding: "6px 14px", fontSize: 10, letterSpacing: 1, textTransform: "uppercase", fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, border: `1px solid ${C.border2}`, borderRadius: 5, cursor: "pointer", background: "transparent", color: C.accent }}>
                ⚙ Schedule & Alerts
              </button>
              {schedule && (
                <span style={{ fontSize: 10, color: C.green, letterSpacing: 0.5 }}>● {schedule === "onemin" ? "Every Min" : schedule === "hourly" ? "Hourly" : schedule === "daily" ? "Daily" : schedule === "weekly" ? "Weekly" : schedule === "monthly" ? "Monthly" : schedule}</span>
              )}
              {alertConfig && (
                <span style={{ fontSize: 10, color: C.orange, letterSpacing: 0.5 }}>📧 Alerts</span>
              )}
            </div>
          </div>
        </div>

        {error && (
          <div style={{ background: "rgba(255,71,87,0.1)", border: `1px solid rgba(255,71,87,0.3)`, borderRadius: 8, padding: "12px 16px", marginBottom: "1.5rem", fontSize: 12, color: C.red }}>
            {error}
          </div>
        )}

        {loading && !records.length ? <Spinner /> : (
          <>
            {/* Stat cards */}
            <div style={{ display: "grid", gridTemplateColumns: "repeat(4, 1fr)", gap: 12, marginBottom: "1.5rem" }}>
              {METRICS.map(m => <StatCard key={m.id} label={m.label} value={statValue(m)} delta={statDelta(m)} anomaly={statAnomalous(m)} />)}
            </div>

            {/* Chart */}
            <div style={{ background: C.bg2, border: `1px solid ${C.border}`, borderRadius: 8, padding: "1.5rem", marginBottom: "1.5rem" }}>
              <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", marginBottom: "1.5rem" }}>
                <div style={{ display: "flex", alignItems: "center", gap: 10 }}>
                  <div style={{ fontFamily: "'Syne', sans-serif", fontSize: 13, fontWeight: 700 }}>Performance Over Time</div>
                  {/* Custom time-range dropdown */}
                  <div ref={dropdownRef} style={{ position: "relative" }}>
                    <button
                      onClick={() => setDropdownOpen(o => !o)}
                      style={{
                        display: "flex", alignItems: "center", gap: 6,
                        background: C.bg3,
                        border: `1px solid ${dropdownOpen ? C.accent : C.border2}`,
                        borderRadius: 4,
                        color: C.accent,
                        fontSize: 10,
                        padding: "5px 10px",
                        fontFamily: "'JetBrains Mono', monospace",
                        fontWeight: 700,
                        letterSpacing: 1,
                        cursor: "pointer",
                        outline: "none",
                        transition: "border-color 0.15s",
                      }}
                    >
                      {activeRange?.label}
                      <svg width="8" height="5" viewBox="0 0 8 5" style={{ transition: "transform 0.15s", transform: dropdownOpen ? "rotate(180deg)" : "rotate(0deg)" }}>
                        <path d="M0 0l4 5 4-5z" fill={C.accent} />
                      </svg>
                    </button>

                    {dropdownOpen && (
                      <div style={{
                        position: "absolute", top: "calc(100% + 4px)", left: 0, zIndex: 200,
                        background: C.bg2,
                        border: `1px solid ${C.border2}`,
                        borderRadius: 6,
                        overflow: "hidden",
                        minWidth: 110,
                        boxShadow: `0 8px 32px rgba(0,0,0,0.7), 0 0 0 1px ${C.border}`,
                      }}>
                        {TIME_RANGES.map((r, i) => (
                          <button
                            key={r.id}
                            onClick={() => { setTimeRange(r.id); setDropdownOpen(false); }}
                            style={{
                              display: "block", width: "100%", textAlign: "left",
                              padding: "8px 12px",
                              fontSize: 10, letterSpacing: 1,
                              fontFamily: "'JetBrains Mono', monospace",
                              fontWeight: r.id === timeRange ? 700 : 400,
                              color: r.id === timeRange ? C.accent : C.text2,
                              background: r.id === timeRange ? "rgba(0,212,255,0.08)" : "transparent",
                              border: "none",
                              borderTop: i > 0 ? `1px solid ${C.border}` : "none",
                              borderLeft: `2px solid ${r.id === timeRange ? C.accent : "transparent"}`,
                              cursor: "pointer",
                              transition: "background 0.1s, color 0.1s",
                            }}
                            onMouseEnter={e => { if (r.id !== timeRange) { e.currentTarget.style.background = C.bg3; e.currentTarget.style.color = C.text; } }}
                            onMouseLeave={e => { if (r.id !== timeRange) { e.currentTarget.style.background = "transparent"; e.currentTarget.style.color = C.text2; } }}
                          >
                            {r.label}
                          </button>
                        ))}
                      </div>
                    )}
                  </div>
                </div>
                <div style={{ display: "flex", gap: 4, background: C.bg3, padding: 3, borderRadius: 6, border: `1px solid ${C.border}` }}>
                  {METRICS.map(m => (
                    <button key={m.id} onClick={() => setActiveMetric(m.id)}
                      style={{ padding: "5px 12px", fontSize: 10, letterSpacing: 1, textTransform: "uppercase", border: "none", borderRadius: 4, cursor: "pointer", fontFamily: "'JetBrains Mono', monospace", background: activeMetric === m.id ? C.accent : "transparent", color: activeMetric === m.id ? C.bg : C.text3, fontWeight: activeMetric === m.id ? 700 : 400 }}>
                      {m.label}
                    </button>
                  ))}
                </div>
              </div>

              {filteredSorted.length === 0 ? (
                <div style={{ textAlign: "center", padding: "4rem", color: C.text3, fontSize: 12 }}>
                  <div style={{ fontSize: 32, marginBottom: 12, opacity: 0.5 }}>◎</div>
                  {records.length === 0
                    ? <>No data yet. Run <code style={{ color: C.accent }}>bash perf_test.sh 500</code> to add your first result.</>
                    : <>No data in the last <span style={{ color: C.accent }}>{activeRange?.label}</span>. Try a wider time range.</>
                  }
                </div>
              ) : (
                <ResponsiveContainer width="100%" height={280}>
                  <LineChart data={chartData} margin={{ top: 8, right: 8, bottom: 8, left: 0 }}>
                    <CartesianGrid stroke={C.border} strokeOpacity={0.5} />
                    <XAxis dataKey="_label" tick={{ fill: C.text3, fontSize: 10, fontFamily: "'JetBrains Mono'" }} tickLine={false} axisLine={{ stroke: C.border }} interval="preserveStartEnd" />
                    <YAxis tick={{ fill: C.text3, fontSize: 10, fontFamily: "'JetBrains Mono'" }} tickLine={false} axisLine={false} />
                    <Tooltip content={<CustomTooltip />} />
                    {baselineVal != null && <ReferenceLine y={baselineVal} stroke={C.text3} strokeDasharray="6 4" strokeWidth={1.5} />}
                    <Line type="monotone" dataKey={metric.field} stroke={C.accent} strokeWidth={2} activeDot={false}
                      dot={(props) => <AnomalyDot {...props} baseline={localBaseline} metricField={metric.field} lowerBetter={metric.lowerBetter} onClick={(p) => setSelected(p._original)} />}
                    />
                  </LineChart>
                </ResponsiveContainer>
              )}

              <div style={{ display: "flex", gap: 16, marginTop: 12, paddingTop: 12, borderTop: `1px solid ${C.border}` }}>
                {[
                  { el: <div style={{ width: 20, height: 2, background: C.accent }} />,                           label: "Metric value" },
                  { el: <div style={{ width: 20, height: 0, borderTop: `2px dashed ${C.text3}` }} />,            label: "Baseline avg" },
                  { el: <div style={{ width: 8, height: 8, borderRadius: "50%", background: C.green }} />,        label: "Normal" },
                  { el: <div style={{ width: 8, height: 8, borderRadius: "50%", background: C.red }} />,          label: "Anomaly" },
                ].map(({ el, label }) => (
                  <div key={label} style={{ display: "flex", alignItems: "center", gap: 6, fontSize: 10, color: C.text3 }}>{el} {label}</div>
                ))}
              </div>
            </div>

            {/* Run history */}
            <div style={{ background: C.bg2, border: `1px solid ${C.border}`, borderRadius: 8, padding: "1.2rem" }}>
              <div style={{ fontSize: 10, letterSpacing: 2, textTransform: "uppercase", color: C.text3, marginBottom: "1rem", display: "flex", alignItems: "center", gap: 8 }}>
                Run History
                <div style={{ flex: 1, height: 1, background: C.border }} />
                <span>
                  {filteredSorted.length !== records.length
                    ? `${filteredSorted.length} / ${records.length} total`
                    : `${records.length} total`}
                </span>
              </div>
              <div style={{ display: "flex", flexDirection: "column", gap: 6, maxHeight: 320, overflowY: "auto" }}>
                {filteredSorted.length === 0 ? (
                  <div style={{ textAlign: "center", padding: "2rem", color: C.text3, fontSize: 12, lineHeight: 2 }}>
                    <div style={{ fontSize: 28, marginBottom: 8, opacity: 0.5 }}>◎</div>
                    {records.length === 0 ? "No runs yet." : `No runs in the last ${activeRange?.label}.`}
                  </div>
                ) : (
                  [...filteredSorted].reverse().map((r, i) => {
                    let _a = r.analysis;
                    if (typeof _a === "string") { try { _a = JSON.parse(_a); } catch { _a = null; } }
                    const status      = _a?.overall_status;
                    const anomalies   = detectAnomalies(r, localBaseline);
                    const isAnomalous = anomalies.some(a => a.worse);
                    const dotColor    = status ? statusColor(status) : (isAnomalous ? C.red : C.green);
                    const d           = new Date(r.timestamp);
                    const time        = d.toLocaleDateString("en-US", { month: "short", day: "numeric" }) + " " + d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
                    return (
                      <div key={i} onClick={() => setSelected(r)}
                        style={{ display: "flex", alignItems: "center", gap: 10, padding: "8px 12px", background: C.bg3, border: `1px solid ${isAnomalous ? "rgba(255,71,87,0.3)" : C.border}`, borderRadius: 5, cursor: "pointer", fontSize: 11 }}>
                        <div style={{ width: 7, height: 7, borderRadius: "50%", background: dotColor, boxShadow: `0 0 6px ${dotColor}`, flexShrink: 0 }} />
                        <div style={{ color: C.text3, flexShrink: 0, minWidth: 120 }}>{time}</div>
                        <div style={{ color: C.text2, flex: 1 }}>{r.avg_latency_ms?.toFixed(2)}ms · {r.throughput_rps?.toFixed(2)}r/s · {r.success_rate}%</div>
                        {r.version && <div style={{ color: C.text3, fontSize: 10, maxWidth: 180, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{r.version}</div>}
                        {status && status !== "stable" && <div style={{ color: dotColor, fontSize: 10, letterSpacing: 1, flexShrink: 0 }}>{statusLabel(status)}</div>}
                      </div>
                    );
                  })
                )}
              </div>
            </div>
          </>
        )}
      </div>

      {selected && <DetailOverlay record={selected} baseline={localBaseline} timeRange={activeRange} onClose={() => setSelected(null)} onDelete={async () => { await fetchData(); setNotification("deleted"); setTimeout(() => setNotification(null), 4000); }} />}

      {notification && (
        <div style={{
          position: "fixed", bottom: 24, right: 24, zIndex: 1000,
          background: C.bg2, border: `1px solid ${notifColor}`,
          borderRadius: 8, padding: "14px 18px", minWidth: 240,
          boxShadow: `0 4px 24px rgba(0,0,0,0.5)`,
          display: "flex", alignItems: "center", gap: 12,
          fontFamily: "'JetBrains Mono', monospace", fontSize: 12,
          animation: "slideIn 0.2s ease",
        }}>
          {notification === "running" && (
            <div style={{ width: 14, height: 14, border: `2px solid ${C.accent}`, borderTopColor: "transparent", borderRadius: "50%", animation: "spin 0.8s linear infinite", flexShrink: 0 }} />
          )}
          {(notification === "completed" || notification === "deleted") && (
            <span style={{ color: C.green, fontSize: 16, flexShrink: 0 }}>✓</span>
          )}
          {notification === "error" && (
            <span style={{ color: C.red, fontSize: 16, flexShrink: 0 }}>✗</span>
          )}
          {notification === "chart_updated" && (
            <span style={{ color: C.accent, fontSize: 16, flexShrink: 0 }}>↻</span>
          )}
          <div>
            <div style={{ color: notifColor, fontWeight: 700, letterSpacing: 1, textTransform: "uppercase", fontSize: 10 }}>
              {notification === "running"       ? "Test Running"    :
               notification === "completed"     ? "Test Completed"  :
               notification === "deleted"       ? "Test Deleted"    :
               notification === "chart_updated" ? "Chart Updated"   :
               "Test Failed"}
            </div>
            <div style={{ color: C.text2, marginTop: 2 }}>
              {notification === "running"       ? "Running perf_test.sh…"             :
               notification === "completed"     ? "Chart refreshed with new results." :
               notification === "deleted"       ? "Run history updated."              :
               notification === "chart_updated" ? `Showing data for ${activeRange?.label}.` :
               "Check server logs for details."}
            </div>
          </div>
        </div>
      )}

      <ScheduleModal
        isOpen={scheduleModalOpen}
        onClose={() => setScheduleModalOpen(false)}
        scheduleInput={scheduleInput}
        setScheduleInput={setScheduleInput}
        alertEmail={alertEmail}
        setAlertEmail={setAlertEmail}
        alertThresholds={alertThresholds}
        setAlertThresholds={setAlertThresholds}
        onSave={saveSchedule}
        onDelete={deleteSchedule}
        saving={scheduleSaving}
        schedule={schedule}
        alertConfig={alertConfig}
      />

      <style>{`
        @keyframes spin { to { transform: rotate(360deg); } }
        @keyframes slideIn { from { opacity: 0; transform: translateY(12px); } to { opacity: 1; transform: translateY(0); } }
      `}</style>
    </div>
  );
}
