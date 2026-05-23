import { METRICS, C } from "./constants";

export function flattenRecord(r) {
  return { ...r, consensus_ms_mean: r.consensus_time_ms?.mean ?? null };
}

export function detectAnomalies(record, baseline) {
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

export function statusColor(status) {
  switch (status) {
    case "possible_regression": return C.red;
    case "needs_attention":     return C.orange;
    case "minor_warning":       return C.orange;
    default:                    return C.green;
  }
}

export function statusLabel(status) {
  switch (status) {
    case "possible_regression": return "⚠ POSSIBLE REGRESSION";
    case "needs_attention":     return "⚠ NEEDS ATTENTION";
    case "minor_warning":       return "⚡ MINOR WARNING";
    default:                    return "✓ STABLE";
  }
}
