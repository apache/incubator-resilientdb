export const API_URL = "";

export const METRICS = [
  { id: "latency",    label: "Latency",    unit: "ms",  field: "avg_latency_ms",    lowerBetter: true  },
  { id: "consensus",  label: "Consensus",  unit: "ms",  field: "consensus_ms_mean", lowerBetter: true  },
  { id: "throughput", label: "Throughput", unit: "r/s", field: "throughput_rps",    lowerBetter: false },
  { id: "success",    label: "Success",    unit: "%",   field: "success_rate",       lowerBetter: false },
];

export const C = {
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
