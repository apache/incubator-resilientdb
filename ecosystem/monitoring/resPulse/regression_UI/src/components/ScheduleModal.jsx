import { C } from "../constants";

export default function ScheduleModal({
  isOpen,
  onClose,
  scheduleInput,
  setScheduleInput,
  alertEmail,
  setAlertEmail,
  alertThresholds,
  setAlertThresholds,
  baselinePeriod,
  setBaselinePeriod,
  baselineAvailability,
  onSave,
  onDelete,
  saving,
  schedule,
  alertConfig
}) {
  const availableMetrics = [
    { key: "latency", label: "Latency" },
    { key: "consensus", label: "Consensus" },
    { key: "throughput", label: "Throughput" },
    { key: "success", label: "Success Rate" }
  ];

  const addMetric = () => {
    const usedMetrics = alertThresholds.map(t => t.metric);
    const availableMetric = availableMetrics.find(m => !usedMetrics.includes(m.key));
    if (availableMetric) {
      setAlertThresholds([...alertThresholds, { metric: availableMetric.key, threshold: 10 }]);
    }
  };

  const removeMetric = (index) => {
    setAlertThresholds(alertThresholds.filter((_, i) => i !== index));
  };

  const updateMetric = (index, field, value) => {
    const updated = [...alertThresholds];
    updated[index] = { ...updated[index], [field]: value };
    setAlertThresholds(updated);
  };

  const getAvailableMetricsForSelect = (currentMetric) => {
    const usedMetrics = alertThresholds.map(t => t.metric).filter(m => m !== currentMetric);
    return availableMetrics.filter(m => !usedMetrics.includes(m.key));
  };
  if (!isOpen) return null;

  const handleBackdropClick = (e) => {
    if (e.target === e.currentTarget) {
      onClose();
    }
  };

  return (
    <div
      style={{
        position: "fixed",
        inset: 0,
        background: "rgba(0,0,0,0.7)",
        backdropFilter: "blur(4px)",
        zIndex: 1000,
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        padding: "2rem"
      }}
      onClick={handleBackdropClick}
    >
      <div style={{
        background: C.bg2,
        border: `1px solid ${C.border}`,
        borderRadius: 12,
        padding: "2rem",
        minWidth: 480,
        maxWidth: "90vw",
        maxHeight: "80vh",
        overflowY: "auto",
        fontFamily: "'JetBrains Mono', monospace"
      }}>
        <div style={{
          display: "flex",
          alignItems: "center",
          justifyContent: "space-between",
          marginBottom: "1.5rem",
          paddingBottom: "1rem",
          borderBottom: `1px solid ${C.border}`
        }}>
          <h2 style={{
            margin: 0,
            fontSize: 16,
            color: C.accent,
            letterSpacing: 1,
            textTransform: "uppercase",
            fontWeight: 700
          }}>
            Schedule & Alerts
          </h2>
          <button
            onClick={onClose}
            style={{
              background: "transparent",
              border: "none",
              color: C.text3,
              fontSize: 18,
              cursor: "pointer",
              padding: "4px 8px",
              borderRadius: 4
            }}
          >
            ×
          </button>
        </div>

        {/* Schedule Section */}
        <div style={{ marginBottom: "2rem" }}>
          <h3 style={{
            margin: "0 0 1rem 0",
            fontSize: 12,
            color: C.text,
            letterSpacing: 1,
            textTransform: "uppercase",
            fontWeight: 600
          }}>
            Test Schedule
          </h3>

          <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
            <select
              value={scheduleInput}
              onChange={e => setScheduleInput(e.target.value)}
              style={{
                background: C.bg3,
                border: `1px solid ${C.border2}`,
                color: scheduleInput === "off" ? C.text3 : C.text,
                fontFamily: "'JetBrains Mono', monospace",
                fontSize: 12,
                padding: "10px 12px",
                borderRadius: 6,
                cursor: "pointer",
                width: "100%"
              }}
            >
              <option value="off">Disabled</option>
              <option value="onemin">Every Minute</option>
              <option value="hourly">Every Hour</option>
              <option value="daily">Every Day</option>
              <option value="weekly">Every Week</option>
              <option value="monthly">Every Month</option>
            </select>

            {schedule && (
              <div style={{
                background: `${C.green}1a`,
                border: `1px solid ${C.green}40`,
                borderRadius: 6,
                padding: "8px 12px",
                fontSize: 11,
                color: C.green,
                display: "flex",
                alignItems: "center",
                gap: 8
              }}>
                <span style={{ fontSize: 8 }}>●</span>
                Currently scheduled: {schedule === "onemin" ? "Every Minute" :
                                      schedule === "hourly" ? "Every Hour" :
                                      schedule === "daily" ? "Every Day" :
                                      schedule === "weekly" ? "Every Week" :
                                      schedule === "monthly" ? "Every Month" : schedule}
              </div>
            )}
          </div>
        </div>

        {/* Baseline Period Section */}
        {scheduleInput !== "off" && (
          <div style={{ marginBottom: "2rem" }}>
            <h3 style={{
              margin: "0 0 1rem 0",
              fontSize: 12,
              color: C.text,
              letterSpacing: 1,
              textTransform: "uppercase",
              fontWeight: 600
            }}>
              Baseline Comparison Period
            </h3>

            <select
              value={baselinePeriod}
              onChange={e => setBaselinePeriod(e.target.value)}
              style={{
                background: C.bg3,
                border: `1px solid ${C.border2}`,
                color: C.text,
                fontFamily: "'JetBrains Mono', monospace",
                fontSize: 12,
                padding: "10px 12px",
                borderRadius: 6,
                cursor: "pointer",
                width: "100%"
              }}
            >
              <option value="1week" disabled={!baselineAvailability?.["1week"]?.available}>
                Last 1 Week {!baselineAvailability?.["1week"]?.available ? "(Insufficient Data)" : `(${baselineAvailability?.["1week"]?.count || 0} results)`}
              </option>
              <option value="1month" disabled={!baselineAvailability?.["1month"]?.available}>
                Last 1 Month {!baselineAvailability?.["1month"]?.available ? "(Insufficient Data)" : `(${baselineAvailability?.["1month"]?.count || 0} results)`}
              </option>
              <option value="3months" disabled={!baselineAvailability?.["3months"]?.available}>
                Last 3 Months {!baselineAvailability?.["3months"]?.available ? "(Insufficient Data)" : `(${baselineAvailability?.["3months"]?.count || 0} results)`}
              </option>
              <option value="6months" disabled={!baselineAvailability?.["6months"]?.available}>
                Last 6 Months {!baselineAvailability?.["6months"]?.available ? "(Insufficient Data)" : `(${baselineAvailability?.["6months"]?.count || 0} results)`}
              </option>
              <option value="1year" disabled={!baselineAvailability?.["1year"]?.available}>
                Last 1 Year {!baselineAvailability?.["1year"]?.available ? "(Insufficient Data)" : `(${baselineAvailability?.["1year"]?.count || 0} results)`}
              </option>
            </select>

            <div style={{
              fontSize: 10,
              color: C.text3,
              marginTop: 8,
              letterSpacing: 0.5,
              lineHeight: 1.4
            }}>
              Performance tests will be compared against results from this time period.
              At least 3 historical results are required for meaningful baseline comparison.
            </div>
          </div>
        )}

        {/* Alert Section */}
        {scheduleInput !== "off" && (
          <div style={{ marginBottom: "2rem" }}>
            <h3 style={{
              margin: "0 0 1rem 0",
              fontSize: 12,
              color: C.text,
              letterSpacing: 1,
              textTransform: "uppercase",
              fontWeight: 600
            }}>
              Regression Alerts
            </h3>

            <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>
              <div>
                <label style={{
                  display: "block",
                  fontSize: 11,
                  color: C.text3,
                  marginBottom: 6,
                  letterSpacing: 0.5
                }}>
                  Email Address
                </label>
                <input
                  type="email"
                  value={alertEmail}
                  onChange={e => setAlertEmail(e.target.value)}
                  placeholder="your@email.com"
                  style={{
                    background: C.bg3,
                    border: `1px solid ${C.border2}`,
                    color: C.text,
                    fontFamily: "'JetBrains Mono', monospace",
                    fontSize: 12,
                    padding: "10px 12px",
                    borderRadius: 6,
                    width: "100%",
                    boxSizing: "border-box"
                  }}
                />
              </div>

              {alertEmail.trim() && (
                <div>
                  <div style={{
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "space-between",
                    marginBottom: 12
                  }}>
                    <div style={{
                      fontSize: 11,
                      color: C.text3,
                      letterSpacing: 0.5
                    }}>
                      Alert when metrics regress by more than:
                    </div>
                    {alertThresholds.length < availableMetrics.length && (
                      <button
                        onClick={addMetric}
                        style={{
                          background: `${C.accent}1a`,
                          border: `1px solid ${C.accent}40`,
                          color: C.accent,
                          fontSize: 10,
                          padding: "4px 8px",
                          borderRadius: 4,
                          cursor: "pointer",
                          fontFamily: "'JetBrains Mono', monospace",
                          letterSpacing: 0.5
                        }}
                      >
                        + Add Metric
                      </button>
                    )}
                  </div>

                  <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                    {alertThresholds.map((threshold, index) => {
                      const availableForSelect = getAvailableMetricsForSelect(threshold.metric);
                      availableForSelect.push(availableMetrics.find(m => m.key === threshold.metric)); // Include current metric

                      return (
                        <div key={index} style={{
                          display: "flex",
                          alignItems: "center",
                          gap: 12,
                          padding: "8px 12px",
                          background: C.bg3,
                          borderRadius: 6,
                          border: `1px solid ${C.border2}`
                        }}>
                          <select
                            value={threshold.metric}
                            onChange={e => updateMetric(index, 'metric', e.target.value)}
                            style={{
                              background: C.bg,
                              border: `1px solid ${C.border}`,
                              color: C.text,
                              fontFamily: "'JetBrains Mono', monospace",
                              fontSize: 11,
                              padding: "4px 8px",
                              borderRadius: 4,
                              minWidth: 100
                            }}
                          >
                            {availableForSelect.map(metric => (
                              <option key={metric.key} value={metric.key}>
                                {metric.label}
                              </option>
                            ))}
                          </select>

                          <div style={{ display: "flex", alignItems: "center", gap: 4 }}>
                            <input
                              type="number"
                              min="1"
                              max="100"
                              value={threshold.threshold}
                              onChange={e => updateMetric(index, 'threshold', parseInt(e.target.value) || 1)}
                              style={{
                                background: C.bg,
                                border: `1px solid ${C.border}`,
                                color: C.text,
                                fontFamily: "'JetBrains Mono', monospace",
                                fontSize: 11,
                                padding: "4px 8px",
                                borderRadius: 4,
                                width: "60px",
                                textAlign: "center"
                              }}
                            />
                            <span style={{ fontSize: 10, color: C.text3 }}>%</span>
                          </div>

                          {alertThresholds.length > 1 && (
                            <button
                              onClick={() => removeMetric(index)}
                              style={{
                                background: "transparent",
                                border: `1px solid ${C.red}40`,
                                color: C.red,
                                fontSize: 10,
                                padding: "4px 8px",
                                borderRadius: 4,
                                cursor: "pointer",
                                fontFamily: "'JetBrains Mono', monospace",
                                letterSpacing: 0.5
                              }}
                            >
                              Remove
                            </button>
                          )}
                        </div>
                      );
                    })}
                  </div>
                </div>
              )}

              {alertConfig && (
                <div style={{
                  background: `${C.green}1a`,
                  border: `1px solid ${C.green}40`,
                  borderRadius: 6,
                  padding: "8px 12px",
                  fontSize: 11,
                  color: C.green,
                  display: "flex",
                  alignItems: "center",
                  gap: 8
                }}>
                  <span style={{ fontSize: 8 }}>●</span>
                  Alerts configured for {alertConfig.email}
                </div>
              )}
            </div>
          </div>
        )}

        {/* Action Buttons */}
        <div style={{
          display: "flex",
          gap: 12,
          justifyContent: schedule ? "space-between" : "flex-end",
          paddingTop: "1rem",
          borderTop: `1px solid ${C.border}`
        }}>
          {schedule && (
            <button
              onClick={onDelete}
              disabled={saving}
              style={{
                background: "transparent",
                border: `1px solid ${C.red}40`,
                color: C.red,
                fontFamily: "'JetBrains Mono', monospace",
                fontSize: 11,
                padding: "8px 16px",
                borderRadius: 6,
                cursor: saving ? "not-allowed" : "pointer",
                letterSpacing: 0.5,
                textTransform: "uppercase",
                fontWeight: 600
              }}
            >
              🗑 Delete Schedule
            </button>
          )}

          <div style={{ display: "flex", gap: 12 }}>
            <button
              onClick={onClose}
              style={{
                background: "transparent",
                border: `1px solid ${C.border2}`,
                color: C.text3,
                fontFamily: "'JetBrains Mono', monospace",
                fontSize: 11,
                padding: "8px 16px",
                borderRadius: 6,
                cursor: "pointer",
                letterSpacing: 0.5,
                textTransform: "uppercase",
                fontWeight: 600
              }}
            >
              Cancel
            </button>
            <button
              onClick={onSave}
              disabled={saving}
              style={{
                background: saving ? C.bg3 : `${C.accent}1a`,
                border: `1px solid ${saving ? C.border2 : C.accent}40`,
                color: saving ? C.text3 : C.accent,
                fontFamily: "'JetBrains Mono', monospace",
                fontSize: 11,
                padding: "8px 16px",
                borderRadius: 6,
                cursor: saving ? "not-allowed" : "pointer",
                letterSpacing: 0.5,
                textTransform: "uppercase",
                fontWeight: 600
              }}
            >
              {saving ? "Saving..." : "Save Configuration"}
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}