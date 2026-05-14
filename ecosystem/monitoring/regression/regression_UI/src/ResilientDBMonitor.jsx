/**
 * ResilientDBMonitor.jsx
 * 
 * Main performance monitoring dashboard component for Apache ResilientDB.
 * Displays real-time and historical performance metrics for the PBFT consensus protocol,
 * including latency, throughput, and success rate. Features include:
 * - Live connection status and performance metrics via stat cards
 * - Interactive line charts showing performance trends over time
 * - Anomaly detection highlighting unusual performance values
 * - Baseline comparison for detecting performance regressions
 * - Run history with detailed information overlay
 * - Auto-refresh with manual refresh capability
 * - Dark-themed UI with grid background and monospace font
 */

import { useState, useEffect, useCallback } from "react";
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

export default function ResilientDBMonitor() {
  const [records,      setRecords]      = useState([]);
  const [apiBaseline,  setApiBaseline]  = useState(null);
  const [loading,      setLoading]      = useState(true);
  const [error,        setError]        = useState(null);
  const [activeMetric, setActiveMetric] = useState("latency");
  const [selected,     setSelected]     = useState(null);
  const [lastRefresh,  setLastRefresh]  = useState(null);

  const fetchData = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const [resultsRes, baselineRes] = await Promise.all([
        fetch(`${API_URL}/api/results?limit=200`),
        fetch(`${API_URL}/api/results/baseline`),
      ]);
      const resultsJson  = await resultsRes.json();
      const baselineJson = await baselineRes.json();
      if (resultsJson.success)                        setRecords(resultsJson.data);
      if (baselineJson.success && baselineJson.data)  setApiBaseline(baselineJson.data);
      setLastRefresh(new Date());
    } catch {
      setError(`Could not connect to backend at ${API_URL}. Make sure the server is running.`);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => { fetchData(); }, [fetchData]);

  const sorted = [...records].sort((a, b) => new Date(a.timestamp) - new Date(b.timestamp));
  const latest = sorted[sorted.length - 1] ?? null;
  const metric = METRICS.find(m => m.id === activeMetric);

  const localBaseline = (() => {
    if (apiBaseline) return apiBaseline;
    if (records.length < 2) return null;
    const bl = {};
    METRICS.forEach(({ field }) => {
      const vals = sorted.map(r => flattenRecord(r)[field]).filter(v => v != null);
      if (!vals.length) return;
      const mean   = vals.reduce((a, b) => a + b, 0) / vals.length;
      const stddev = Math.sqrt(vals.reduce((a, b) => a + (b - mean) ** 2, 0) / vals.length);
      bl[field] = { mean, stddev };
    });
    return bl;
  })();

  const chartData   = sorted.map(r => ({ ...flattenRecord(r), _label: new Date(r.timestamp).toLocaleDateString("en-US", { month: "short", day: "numeric" }), _original: r }));
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

  return (
    <div style={{ background: C.bg, minHeight: "100vh", fontFamily: "'JetBrains Mono', monospace", color: C.text, position: "relative", overflowX: "hidden" }}>
      <div style={{ position: "fixed", inset: 0, backgroundImage: `linear-gradient(rgba(0,212,255,0.03) 1px, transparent 1px), linear-gradient(90deg, rgba(0,212,255,0.03) 1px, transparent 1px)`, backgroundSize: "40px 40px", pointerEvents: "none", zIndex: 0 }} />

      <div style={{ position: "relative", zIndex: 1, maxWidth: 1200, margin: "0 auto", padding: "2rem 1.5rem" }}>

        {/* Header */}
        <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", marginBottom: "2.5rem", paddingBottom: "1.5rem", borderBottom: `1px solid ${C.border}` }}>
          <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
            <span style={{ fontSize: 10, color: C.accent, letterSpacing: 3, textTransform: "uppercase", fontWeight: 500 }}>Apache ResilientDB</span>
            <span style={{ fontFamily: "'Syne', sans-serif", fontSize: 22, fontWeight: 800, letterSpacing: -0.5 }}>Performance Monitor</span>
            <span style={{ fontSize: 11, color: C.text3, letterSpacing: 1 }}>PBFT Consensus Analytics</span>
          </div>
          <div style={{ textAlign: "right", display: "flex", flexDirection: "column", alignItems: "flex-end", gap: 8 }}>
            <div style={{ display: "flex", alignItems: "center", gap: 8, fontSize: 11, color: C.text2 }}>
              <span style={{ width: 6, height: 6, borderRadius: "50%", background: error ? C.red : C.green, boxShadow: `0 0 8px ${error ? C.red : C.green}`, display: "inline-block" }} />
              {error ? "DISCONNECTED" : "LIVE"}
            </div>
            <div style={{ fontSize: 11, color: C.text3 }}>{records.length} run{records.length !== 1 ? "s" : ""} in database</div>
            {lastRefresh && <div style={{ fontSize: 10, color: C.text3 }}>Last refresh: {lastRefresh.toLocaleTimeString()}</div>}
            <button onClick={fetchData} disabled={loading}
              style={{ padding: "6px 14px", fontSize: 10, letterSpacing: 1, textTransform: "uppercase", fontFamily: "'JetBrains Mono', monospace", fontWeight: 700, border: `1px solid ${C.border2}`, borderRadius: 5, cursor: loading ? "not-allowed" : "pointer", background: "transparent", color: loading ? C.text3 : C.accent }}>
              {loading ? "Loading..." : "↻ Refresh"}
            </button>
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
                <div style={{ fontFamily: "'Syne', sans-serif", fontSize: 13, fontWeight: 700 }}>Performance Over Time</div>
                <div style={{ display: "flex", gap: 4, background: C.bg3, padding: 3, borderRadius: 6, border: `1px solid ${C.border}` }}>
                  {METRICS.map(m => (
                    <button key={m.id} onClick={() => setActiveMetric(m.id)}
                      style={{ padding: "5px 12px", fontSize: 10, letterSpacing: 1, textTransform: "uppercase", border: "none", borderRadius: 4, cursor: "pointer", fontFamily: "'JetBrains Mono', monospace", background: activeMetric === m.id ? C.accent : "transparent", color: activeMetric === m.id ? C.bg : C.text3, fontWeight: activeMetric === m.id ? 700 : 400 }}>
                      {m.label}
                    </button>
                  ))}
                </div>
              </div>

              {records.length === 0 ? (
                <div style={{ textAlign: "center", padding: "4rem", color: C.text3, fontSize: 12 }}>
                  <div style={{ fontSize: 32, marginBottom: 12, opacity: 0.5 }}>◎</div>
                  No data yet. Run <code style={{ color: C.accent }}>bash perf_test.sh 500</code> to add your first result.
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
                Run History <div style={{ flex: 1, height: 1, background: C.border }} /> <span>{records.length} total</span>
              </div>
              <div style={{ display: "flex", flexDirection: "column", gap: 6, maxHeight: 320, overflowY: "auto" }}>
                {sorted.length === 0 ? (
                  <div style={{ textAlign: "center", padding: "2rem", color: C.text3, fontSize: 12, lineHeight: 2 }}>
                    <div style={{ fontSize: 28, marginBottom: 8, opacity: 0.5 }}>◎</div>
                    No runs yet.
                  </div>
                ) : (
                  [...sorted].reverse().map((r, i) => {
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

      {selected && <DetailOverlay record={selected} baseline={localBaseline} onClose={() => setSelected(null)} />}
    </div>
  );
}
