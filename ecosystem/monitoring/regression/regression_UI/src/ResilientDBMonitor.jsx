/**
 * ResilientDB Performance Monitor
 *
 * Fetches all data from MongoDB via the backend API.
 * Run perf_test.sh to add new results — they save automatically.
 *
 * Usage:
 *   1. npm install recharts
 *   2. Set REACT_APP_API_URL in your .env (default: http://localhost:5000)
 *   3. Import and render: <ResilientDBMonitor />
 */

import { useState, useEffect, useCallback } from "react";
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid,
  Tooltip, ReferenceLine, ResponsiveContainer,
} from "recharts";

// ─── Config ───────────────────────────────────────────────────────────────────

const API_URL = "";

// ─── Metrics definition ───────────────────────────────────────────────────────

const METRICS = [
  { id: "latency",    label: "Latency",    unit: "ms",  field: "avg_latency_ms",    lowerBetter: true  },
  { id: "consensus",  label: "Consensus",  unit: "ms",  field: "consensus_ms_mean", lowerBetter: true  },
  { id: "throughput", label: "Throughput", unit: "r/s", field: "throughput_rps",    lowerBetter: false },
  { id: "success",    label: "Success",    unit: "%",   field: "success_rate",       lowerBetter: false },
];

// ─── Colours ──────────────────────────────────────────────────────────────────

const C = {
  bg:      "#0a0c0f",
  bg2:     "#111318",
  bg3:     "#181c23",
  border:  "#1e2430",
  border2: "#252d3a",
  text:    "#e2e8f0",
  text2:   "#7a8aa0",
  text3:   "#4a5568",
  accent:  "#00d4ff",
  green:   "#00e676",
  red:     "#ff4757",
  orange:  "#ffa726",
  purple:  "#b388ff",
};

// ─── Helpers ──────────────────────────────────────────────────────────────────

function flattenRecord(r) {
  return { ...r, consensus_ms_mean: r.consensus_time_ms?.mean ?? null };
}

function detectAnomalies(record, baseline) {
  if (!baseline) return [];
  const flat = flattenRecord(record);
  return METRICS
    .map(({ field, lowerBetter }) => {
      const bval = baseline[field]?.mean;
      const val  = flat[field];
      if (bval == null || val == null) return null;
      const diff = val - bval;
      if (diff === 0) return null;
      return { field, diff, pct: (diff / bval) * 100, worse: lowerBetter ? diff > 0 : diff < 0 };
    })
    .filter(Boolean);
}

function statusColor(status) {
  switch (status) {
    case "possible_regression": return C.red;
    case "needs_attention":     return C.orange;
    case "minor_warning":       return C.orange;
    default:                    return C.green;
  }
}

function statusLabel(status) {
  switch (status) {
    case "possible_regression": return "⚠ POSSIBLE REGRESSION";
    case "needs_attention":     return "⚠ NEEDS ATTENTION";
    case "minor_warning":       return "⚡ MINOR WARNING";
    default:                    return "✓ STABLE";
  }
}

// ─── Small components ─────────────────────────────────────────────────────────

function Tag({ children, color, bg, border }) {
  return (
    <span style={{ display: "inline-flex", alignItems: "center", gap: 5, padding: "3px 10px", borderRadius: 4, fontSize: 10, letterSpacing: 1, fontWeight: 700, color, background: bg, border: `1px solid ${border}` }}>
      {children}
    </span>
  );
}

function StatCard({ label, value, delta, anomaly }) {
  return (
    <div style={{ background: C.bg2, border: `1px solid ${anomaly ? "rgba(255,71,87,0.35)" : C.border}`, borderRadius: 8, padding: "1rem 1.2rem", position: "relative", overflow: "hidden" }}>
      <div style={{ position: "absolute", top: 0, left: 0, right: 0, height: 2, background: anomaly ? C.red : C.accent, opacity: anomaly ? 1 : 0.4 }} />
      <div style={{ fontSize: 10, color: C.text3, letterSpacing: 2, textTransform: "uppercase", marginBottom: 6 }}>{label}</div>
      <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1, marginBottom: 4 }}>{value}</div>
      <div style={{ fontSize: 11 }}>{delta}</div>
    </div>
  );
}

function Spinner() {
  return (
    <div style={{ display: "flex", alignItems: "center", justifyContent: "center", padding: "4rem", color: C.text3, fontSize: 12, gap: 10 }}>
      <div style={{ width: 16, height: 16, borderRadius: "50%", border: `2px solid ${C.border2}`, borderTopColor: C.accent, animation: "spin 0.8s linear infinite" }} />
      Loading from database...
      <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
    </div>
  );
}

function JsonHighlight({ json }) {
  const highlighted = json.replace(
    /("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)/g,
    (match) => {
      let color = C.purple;
      if (/^"/.test(match)) color = /:$/.test(match) ? C.accent : C.green;
      return `<span style="color:${color}">${match}</span>`;
    }
  );
  return (
    <pre style={{ fontSize: 11, lineHeight: 1.6, color: C.text2, overflow: "auto", whiteSpace: "pre" }} dangerouslySetInnerHTML={{ __html: highlighted }} />
  );
}

// ─── Analysis section helpers ─────────────────────────────────────────────────

function Em({ children }) {
  return <strong style={{ color: C.text, fontWeight: 600 }}>{children}</strong>;
}

function AnalysisSection({ label, children }) {
  return (
    <div style={{ background: C.bg3, border: `1px solid ${C.border2}`, borderRadius: 8, padding: "1.2rem 1.4rem", marginBottom: "1.2rem", position: "relative" }}>
      <div style={{ position: "absolute", top: -8, left: 12, fontSize: 9, letterSpacing: 2, color: C.accent, background: C.bg3, padding: "0 6px", fontWeight: 700 }}>{label}</div>
      {children}
    </div>
  );
}

// ─── Detail overlay ───────────────────────────────────────────────────────────

function DetailOverlay({ record, baseline, onClose }) {
  const [jsonOpen, setJsonOpen] = useState(false);
  const flat      = flattenRecord(record);
  const anomalies = detectAnomalies(record, baseline);
  const d         = new Date(record.timestamp);

  let _analysis = record.analysis;
  if (typeof _analysis === "string") {
    try { _analysis = JSON.parse(_analysis); } catch { /* leave as raw string */ }
  }
  const analysis = _analysis;
  const isObject = analysis && typeof analysis === "object" && !Array.isArray(analysis);
  const overallStatus = isObject ? analysis.overall_status : (anomalies.some(a => a.worse) ? "needs_attention" : "stable");
  const color         = statusColor(overallStatus);

  const diagnosisItems  = isObject ? analysis.diagnosis       : (Array.isArray(analysis) ? analysis : [analysis].filter(Boolean));
  const recommendations = isObject ? analysis.recommendations : [];
  const warnings        = isObject ? analysis.warnings        : [];
  const histChanges     = isObject ? analysis.historical_percent_changes    : null;
  const histSummary     = isObject ? analysis.historical_baseline_summary   : null;

  const HIST_KEYWORDS = ["historical baseline", "baseline", "regression"];
  const isHistItem    = (item) => HIST_KEYWORDS.some(kw => item.toLowerCase().includes(kw));
  const systemDiagnosis = diagnosisItems?.filter(item => !isHistItem(item)) ?? [];

  // Pick the most notable system issue (skip neutral/positive observations)
  const NEUTRAL = ["is low", "is relatively stable", "variance is low", "high throughput", "moderate throughput", "latency variance is low", "consistent request"];
  const mainIssue = systemDiagnosis.find(item => !NEUTRAL.some(kw => item.toLowerCase().includes(kw))) ?? systemDiagnosis[0];

  // Pre-compute comparison narrative values
  const latPct  = histChanges?.avg_latency_change_pct;
  const tpPct   = histChanges?.throughput_change_pct;
  const swPct   = histChanges?.server_wait_change_pct;
  const latFrom = histSummary?.historical_avg_latency_ms;
  const tpFrom  = histSummary?.historical_avg_throughput_rps;
  const worseOverall = (latPct ?? 0) > 10 || (tpPct ?? 0) < -10;

  const prose = { fontSize: 13, lineHeight: 1.8, color: C.text2, marginBottom: 10, marginTop: 6 };

  return (
    <div onClick={(e) => e.target === e.currentTarget && onClose()}
      style={{ position: "fixed", inset: 0, background: "rgba(0,0,0,0.85)", backdropFilter: "blur(4px)", zIndex: 200, display: "flex", alignItems: "center", justifyContent: "center", padding: "2rem" }}>
      <div style={{ background: C.bg2, border: `1px solid ${C.border2}`, borderRadius: 10, width: "100%", maxWidth: 720, maxHeight: "88vh", overflowY: "auto" }}>

        {/* Header */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "1.2rem 1.5rem", borderBottom: `1px solid ${C.border}`, position: "sticky", top: 0, background: C.bg2, zIndex: 1 }}>
          <div>
            <div style={{ fontFamily: "'Syne', sans-serif", fontSize: 14, fontWeight: 700 }}>
              {d.toLocaleDateString("en-US", { weekday: "short", month: "long", day: "numeric" })} — {d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" })}
            </div>
            {record.version && <div style={{ fontSize: 10, color: C.text3, marginTop: 3, letterSpacing: 1 }}>{record.version}</div>}
          </div>
          <button onClick={onClose} style={{ background: C.bg3, border: `1px solid ${C.border}`, color: C.text2, width: 28, height: 28, borderRadius: 5, cursor: "pointer", fontSize: 14, display: "flex", alignItems: "center", justifyContent: "center" }}>✕</button>
        </div>

        <div style={{ padding: "1.5rem" }}>

          {/* Status badge + warnings */}
          <div style={{ display: "flex", alignItems: "center", gap: 8, flexWrap: "wrap", marginBottom: "1.4rem" }}>
            <Tag color={color} bg={`${color}1a`} border={`${color}4d`}>{statusLabel(overallStatus)}</Tag>
            {warnings.map((w, i) => (
              <span key={i} style={{ fontSize: 11, color: C.orange, paddingLeft: 8, borderLeft: `2px solid ${C.orange}` }}>{w}</span>
            ))}
          </div>

          {/* Section 1 — What the performance gave us */}
          <AnalysisSection label="WHAT THE PERFORMANCE GAVE US">
            <p style={prose}>
              This run processed <Em>{record.runs?.toLocaleString()} transactions</Em> with
              a <Em>{record.success_rate}% success rate</Em>.
              The system sustained a throughput of <Em>{record.throughput_rps?.toFixed(2)} requests per second</Em> and
              an average end-to-end latency of <Em>{record.avg_latency_ms?.toFixed(2)} ms</Em>.
              At the median (p50), requests completed in <Em>{record.total_latency?.p50 ?? "—"} ms</Em>;
              the slowest 5% took <Em>{record.total_latency?.p95 ?? "—"} ms</Em> or longer,
              and the slowest 1% reached <Em>{record.total_latency?.p99 ?? "—"} ms</Em>.
              {flat.consensus_ms_mean != null && (
                <> The PBFT consensus round-trip averaged <Em>{flat.consensus_ms_mean?.toFixed(2)} ms</Em>.</>
              )}
              {record.tcp_connect_ms?.mean != null && (
                <> TCP connection setup added roughly <Em>{record.tcp_connect_ms.mean.toFixed(2)} ms</Em> per request.</>
              )}
            </p>
            <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 10, marginTop: 14 }}>
              {[
                { label: "Avg Latency",    val: `${record.avg_latency_ms?.toFixed(2)} ms`,             sub: `p95: ${record.total_latency?.p95 ?? "—"} ms · p99: ${record.total_latency?.p99 ?? "—"} ms` },
                { label: "Consensus Time", val: `${flat.consensus_ms_mean?.toFixed(2) ?? "—"} ms`,     sub: `p95: ${record.consensus_time_ms?.p95 ?? "—"} ms · p99: ${record.consensus_time_ms?.p99 ?? "—"} ms` },
                { label: "Throughput",     val: `${record.throughput_rps?.toFixed(2)} r/s`,             sub: `${record.runs} transactions · ${record.success_rate}% success` },
                { label: "TCP Connect",    val: `${record.tcp_connect_ms?.mean?.toFixed(2) ?? "—"} ms`, sub: `Transfer: ${record.transfer_time_ms?.mean?.toFixed(2) ?? "—"} ms` },
              ].map(({ label, val, sub }) => (
                <div key={label} style={{ background: C.bg, border: `1px solid ${C.border}`, borderRadius: 6, padding: "10px 12px" }}>
                  <div style={{ fontSize: 9, letterSpacing: 2, color: C.text3, textTransform: "uppercase", marginBottom: 4 }}>{label}</div>
                  <div style={{ fontSize: 16, fontWeight: 700, marginBottom: 2 }}>{val}</div>
                  <div style={{ fontSize: 10, color: C.text3 }}>{sub}</div>
                </div>
              ))}
            </div>
          </AnalysisSection>

          {/* Section 2 — What this tells about the system */}
          {systemDiagnosis.length > 0 && (
            <AnalysisSection label="WHAT THIS TELLS ABOUT THE SYSTEM">
              {systemDiagnosis.map((item, i) => (
                <p key={i} style={prose}>{item}</p>
              ))}
            </AnalysisSection>
          )}

          {/* Section 3 — How it compares to previous runs (only when baseline data exists) */}
          {(histSummary || histChanges) && (
            <AnalysisSection label="HOW IT COMPARES TO PREVIOUS RUNS">

              {/* Current run vs. baseline narrative */}
              {histSummary && histChanges && (
                <>
                  <div style={{ fontSize: 10, fontWeight: 700, color: C.text, letterSpacing: 0.5, marginBottom: 4, marginTop: 4 }}>Current run vs. baseline</div>
                  <p style={prose}>
                    This run completed{" "}
                    <Em>
                      {record.success_rate === 100
                        ? `all ${record.runs?.toLocaleString()} transactions successfully`
                        : `${record.runs?.toLocaleString()} transactions with a ${record.success_rate}% success rate`}
                    </Em>
                    {worseOverall
                      ? ", but performance is worse than the historical baseline"
                      : ", and performance is broadly in line with the historical baseline"}.
                    {" "}
                    {latFrom != null && record.avg_latency_ms != null && latPct != null && (
                      <>
                        Average latency{" "}
                        <Em>
                          {latPct > 0 ? "increased" : "decreased"} from {latFrom.toFixed(2)} ms to {record.avg_latency_ms.toFixed(2)} ms ({latPct > 0 ? "+" : "−"}{Math.abs(latPct)}%)
                        </Em>
                        {tpFrom != null && record.throughput_rps != null && tpPct != null && (
                          <>, while throughput{" "}
                          <Em>
                            {tpPct < 0 ? "dropped" : "rose"} from{" "}
                            {Number(tpFrom).toLocaleString("en-US", { minimumFractionDigits: 2, maximumFractionDigits: 2 })} r/s to{" "}
                            {Number(record.throughput_rps).toLocaleString("en-US", { minimumFractionDigits: 2, maximumFractionDigits: 2 })} r/s ({tpPct > 0 ? "+" : "−"}{Math.abs(tpPct)}%)
                          </Em></>
                        )}
                        {swPct != null && Math.abs(swPct) > 5 && (
                          <>. Server-side wait time also{" "}
                          <Em>{swPct > 0 ? "increased" : "decreased"} by {Math.abs(swPct)}%</Em>,
                          {" "}{swPct > 0 ? "suggesting more delay inside the request-processing path" : "suggesting improvement in the request-processing path"}</>
                        )}
                        .
                      </>
                    )}
                  </p>
                </>
              )}

              {/* Main issue */}
              {mainIssue && (
                <>
                  <div style={{ fontSize: 10, fontWeight: 700, color: C.text, letterSpacing: 0.5, marginBottom: 4, marginTop: 14 }}>Main issue</div>
                  <p style={prose}>{mainIssue}</p>
                </>
              )}

              {/* Delta grid */}
              {histChanges && Object.values(histChanges).some(v => v != null) && (
                <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 8, marginTop: 14, paddingTop: 14, borderTop: `1px solid ${C.border}` }}>
                  {[
                    { label: "Latency",     val: latPct, lowerBetter: true  },
                    { label: "Throughput",  val: tpPct,  lowerBetter: false },
                    { label: "Server Wait", val: swPct,  lowerBetter: true  },
                  ].filter(i => i.val != null).map(({ label, val, lowerBetter }) => {
                    const worse = lowerBetter ? val > 0 : val < 0;
                    return (
                      <div key={label} style={{ display: "flex", flexDirection: "column", gap: 2 }}>
                        <span style={{ fontSize: 10, color: C.text3 }}>{label}</span>
                        <span style={{ fontSize: 14, fontWeight: 700, color: worse ? C.red : C.green }}>{val > 0 ? "+" : ""}{val}%</span>
                      </div>
                    );
                  })}
                </div>
              )}

              {histSummary?.record_count != null && (
                <div style={{ fontSize: 10, color: C.text3, marginTop: 10 }}>
                  Based on {histSummary.record_count} historical record{histSummary.record_count !== 1 ? "s" : ""}
                  {histSummary.period_start && <> going back to {new Date(histSummary.period_start).toLocaleDateString("en-US", { month: "long", year: "numeric" })}</>}
                </div>
              )}

            </AnalysisSection>
          )}

          {/* Section 4 — What you can do (shown whenever recommendations exist) */}
          {recommendations.length > 0 && (
            <AnalysisSection label="WHAT YOU CAN DO TO IMPROVE IT">
              <p style={{ ...prose, marginBottom: 12 }}>
                Based on the patterns detected above, here are concrete steps to investigate and improve performance:
              </p>
              {recommendations.map((r, i) => (
                <div key={i} style={{ fontSize: 12, lineHeight: 1.75, color: C.text2, marginBottom: 10, paddingLeft: 14, borderLeft: `2px solid ${C.accent}` }}>
                  {r}
                </div>
              ))}
            </AnalysisSection>
          )}

          {/* JSON toggle */}
          <button onClick={() => setJsonOpen(o => !o)}
            style={{ background: "transparent", border: `1px solid ${C.border2}`, color: C.text3, fontFamily: "'JetBrains Mono', monospace", fontSize: 10, letterSpacing: 1, padding: "6px 12px", borderRadius: 5, cursor: "pointer", width: "100%", textAlign: "left", display: "flex", alignItems: "center", justifyContent: "space-between" }}>
            <span>VIEW FULL JSON</span><span>{jsonOpen ? "▴" : "▾"}</span>
          </button>
          {jsonOpen && (
            <div style={{ background: C.bg, border: `1px solid ${C.border}`, borderRadius: 6, padding: 12, marginTop: 8 }}>
              <JsonHighlight json={JSON.stringify(record, null, 2)} />
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

// ─── Custom chart dot ─────────────────────────────────────────────────────────

function AnomalyDot({ cx, cy, payload, baseline, metricField, lowerBetter, onClick }) {
  if (!cx || !cy) return null;
  const bval = baseline?.[metricField]?.mean;
  let fill = C.accent;
  if (bval != null) {
    const worse = lowerBetter ? payload[metricField] > bval : payload[metricField] < bval;
    fill = worse ? C.red : C.green;
  }
  return <circle cx={cx} cy={cy} r={6} fill={fill} stroke={C.bg2} strokeWidth={2} style={{ cursor: "pointer" }} onClick={() => onClick && onClick(payload)} />;
}

// ─── Main component ───────────────────────────────────────────────────────────

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

  // Use API baseline if available, otherwise compute from records
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
                    const status     = _a?.overall_status;
                    const anomalies  = detectAnomalies(r, localBaseline);
                    const isAnomalous = anomalies.some(a => a.worse);
                    const dotColor   = status ? statusColor(status) : (isAnomalous ? C.red : C.green);
                    const d          = new Date(r.timestamp);
                    const time       = d.toLocaleDateString("en-US", { month: "short", day: "numeric" }) + " " + d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
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