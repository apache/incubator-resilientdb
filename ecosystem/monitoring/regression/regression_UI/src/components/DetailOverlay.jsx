/**
 * DetailOverlay.jsx
 *
 * Modal overlay for a single test run. Performance metrics are always visible.
 * Pattern analysis + historical comparison are generated on demand via the
 * "Generate Analysis" button, which compares against the time window the user
 * is currently viewing in the chart.
 */

import { useState } from "react";
import { C, API_URL } from "../constants";
import { flattenRecord, detectAnomalies, statusColor, statusLabel } from "../utils";
import Tag from "./Tag";
import AnalysisSection, { Em } from "./AnalysisSection";
import JsonHighlight from "./JsonHighlight";

export default function DetailOverlay({ record, baseline, onClose, onDelete, timeRange }) {
  const [jsonOpen,           setJsonOpen]           = useState(false);
  const [confirmOpen,        setConfirmOpen]         = useState(false);
  const [deleting,           setDeleting]            = useState(false);
  const [deleteError,        setDeleteError]         = useState(null);
  const [generatedAnalysis,  setGeneratedAnalysis]   = useState(null);
  const [analyzing,          setAnalyzing]           = useState(false);
  const [analysisError,      setAnalysisError]       = useState(null);

  const flat      = flattenRecord(record);
  const anomalies = detectAnomalies(record, baseline);
  const d         = new Date(record.timestamp);

  const isObject      = generatedAnalysis && typeof generatedAnalysis === "object";
  const overallStatus = isObject
    ? generatedAnalysis.overall_status
    : (anomalies.some(a => a.worse) ? "needs_attention" : "stable");
  const color = statusColor(overallStatus);

  const diagnosisItems  = isObject ? generatedAnalysis.diagnosis                        : [];
  const recommendations = isObject ? generatedAnalysis.recommendations                  : [];
  const warnings        = isObject ? generatedAnalysis.warnings                         : [];
  const histChanges     = isObject ? generatedAnalysis.historical_percent_changes        : null;
  const histSummary     = isObject ? generatedAnalysis.historical_baseline_summary       : null;

  const HIST_KEYWORDS   = ["historical baseline", "baseline", "regression"];
  const isHistItem      = (item) => HIST_KEYWORDS.some(kw => item.toLowerCase().includes(kw));
  const systemDiagnosis = diagnosisItems?.filter(item => !isHistItem(item)) ?? [];

  const NEUTRAL   = ["is low", "is relatively stable", "variance is low", "high throughput", "moderate throughput", "latency variance is low", "consistent request"];
  const mainIssue = systemDiagnosis.find(item => !NEUTRAL.some(kw => item.toLowerCase().includes(kw))) ?? systemDiagnosis[0];

  const latPct       = histChanges?.avg_latency_change_pct;
  const tpPct        = histChanges?.throughput_change_pct;
  const swPct        = histChanges?.server_wait_change_pct;
  const latFrom      = histSummary?.historical_avg_latency_ms;
  const tpFrom       = histSummary?.historical_avg_throughput_rps;
  const worseOverall = (latPct ?? 0) > 10 || (tpPct ?? 0) < -10;

  const prose = { fontSize: 13, lineHeight: 1.8, color: C.text2, marginBottom: 10, marginTop: 6 };

  const periodLabel = timeRange?.label ?? "6 Months";

  async function handleGenerate() {
    const id = record._id || record.id;
    if (!id) return;
    setAnalyzing(true);
    setAnalysisError(null);
    try {
      const res = await fetch(`${API_URL}/api/results/${id}/analyze`, {
        method:  "POST",
        headers: { "Content-Type": "application/json" },
        body:    JSON.stringify({ hours: timeRange?.hours ?? null, periodLabel }),
      });
      const j = await res.json();
      if (!res.ok || !j.success) throw new Error(j.error || `HTTP ${res.status}`);
      setGeneratedAnalysis(j.data);
    } catch (e) {
      setAnalysisError(e.message || String(e));
    } finally {
      setAnalyzing(false);
    }
  }

  return (
    <>
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

            {/* Generate Analysis button — shown until analysis is ready */}
            {!generatedAnalysis && (
              <div style={{ marginTop: 8, marginBottom: 8 }}>
                {analysisError && (
                  <div style={{ fontSize: 12, color: C.red, marginBottom: 10, padding: "8px 12px", background: `${C.red}14`, border: `1px solid ${C.red}40`, borderRadius: 6 }}>
                    {analysisError}
                  </div>
                )}
                <button
                  onClick={handleGenerate}
                  disabled={analyzing}
                  style={{
                    background:    analyzing ? C.bg3 : C.accent + "1a",
                    border:        `1px solid ${analyzing ? C.border2 : C.accent + "4d"}`,
                    color:         analyzing ? C.text3 : C.accent,
                    fontFamily:    "'JetBrains Mono', monospace",
                    fontSize:       11,
                    letterSpacing:  1,
                    padding:       "10px 16px",
                    borderRadius:   6,
                    cursor:         analyzing ? "default" : "pointer",
                    width:          "100%",
                    textAlign:      "left",
                    display:        "flex",
                    alignItems:     "center",
                    gap:            10,
                    transition:     "opacity 0.15s",
                  }}
                >
                  {analyzing
                    ? <><Spinner />  Generating analysis…</>
                    : <>&#9654;&#xFE0E;&nbsp; Generate Analysis — Compare last {periodLabel}</>
                  }
                </button>
              </div>
            )}

            {/* Section 2 — What this tells about the system */}
            {systemDiagnosis.length > 0 && (
              <AnalysisSection label="WHAT THIS TELLS ABOUT THE SYSTEM">
                {systemDiagnosis.map((item, i) => (
                  <p key={i} style={prose}>{item}</p>
                ))}
              </AnalysisSection>
            )}

            {/* Section 3 — How it compares to previous runs */}
            {(histSummary || histChanges) && (
              <AnalysisSection label={`HOW IT COMPARES TO THE LAST ${periodLabel.toUpperCase()}`}>

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

                {mainIssue && (
                  <>
                    <div style={{ fontSize: 10, fontWeight: 700, color: C.text, letterSpacing: 0.5, marginBottom: 4, marginTop: 14 }}>Main issue</div>
                    <p style={prose}>{mainIssue}</p>
                  </>
                )}

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

            {/* Section 4 — What you can do */}
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

            {/* Delete button */}
            <div style={{ display: "flex", justifyContent: "space-between", gap: 8, marginBottom: 8 }}>
              <button onClick={() => setConfirmOpen(true)}
                style={{ background: "#ff3b30", border: `1px solid rgba(255,59,48,0.15)`, color: "#fff", fontFamily: "'JetBrains Mono', monospace", fontSize: 11, letterSpacing: 1, padding: "8px 12px", borderRadius: 6, cursor: "pointer" }}>
                Delete Test
              </button>
              <div style={{ flex: 1 }} />
            </div>

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

      {confirmOpen && (
        <div style={{ position: "fixed", inset: 0, background: "rgba(0,0,0,0.6)", zIndex: 300, display: "flex", alignItems: "center", justifyContent: "center" }}>
          <div style={{ background: C.bg2, border: `1px solid ${C.border}`, borderRadius: 8, padding: 20, width: 420, boxShadow: "0 8px 30px rgba(0,0,0,0.6)" }}>
            <div style={{ fontSize: 14, fontWeight: 700, marginBottom: 8 }}>Delete this test?</div>
            <div style={{ color: C.text2, marginBottom: 14 }}>This action will permanently remove the selected run from the database. Are you sure?</div>
            {deleteError && <div style={{ color: C.red, marginBottom: 8 }}>{deleteError}</div>}
            <div style={{ display: "flex", gap: 8, justifyContent: "flex-end" }}>
              <button onClick={() => setConfirmOpen(false)} disabled={deleting}
                style={{ background: "transparent", border: `1px solid ${C.border}`, color: C.text3, padding: "8px 12px", borderRadius: 6, cursor: "pointer" }}>Cancel</button>
              <button onClick={async () => {
                setDeleting(true);
                setDeleteError(null);
                try {
                  const id = record._id || record.id;
                  if (!id) throw new Error("Record id not found");
                  const resp = await fetch(`${API_URL}/api/results/${id}`, { method: "DELETE" });
                  const j = await resp.json().catch(() => ({}));
                  if (!resp.ok) throw new Error(j.error || `HTTP ${resp.status}`);
                  setConfirmOpen(false);
                  onClose();
                  try { if (onDelete) await onDelete(); } catch (e) { console.warn("onDelete failed", e); }
                } catch (e) {
                  setDeleteError(e.message || String(e));
                } finally {
                  setDeleting(false);
                }
              }} disabled={deleting}
                style={{ background: "#ff3b30", border: `1px solid rgba(255,59,48,0.15)`, color: "#fff", padding: "8px 12px", borderRadius: 6, cursor: "pointer" }}>
                {deleting ? "Deleting…" : "Yes, delete"}
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}

function Spinner() {
  return (
    <span style={{
      display:      "inline-block",
      width:         12,
      height:        12,
      border:        `2px solid currentColor`,
      borderTopColor: "transparent",
      borderRadius:  "50%",
      animation:     "spin 0.7s linear infinite",
      flexShrink:    0,
    }} />
  );
}
