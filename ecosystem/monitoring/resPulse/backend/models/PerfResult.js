const mongoose = require("mongoose");

const PercentileSchema = new mongoose.Schema({
  mean: Number, min: Number, max: Number,
  stddev: Number, p50: Number, p95: Number, p99: Number,
}, { _id: false });

const PerfResultSchema = new mongoose.Schema({
  timestamp:         { type: Date, required: true },
  runs:              { type: Number, required: true },
  success_rate:      { type: Number, required: true },
  throughput_rps:    { type: Number, required: true },
  avg_latency_ms:    { type: Number, required: true },
  total_latency:     PercentileSchema,
  consensus_time_ms: PercentileSchema,
  tcp_connect_ms:    PercentileSchema,
  transfer_time_ms:  PercentileSchema,
  analysis:          { type: String },
  version:           { type: String },
}, { timestamps: true });

PerfResultSchema.index({ timestamp: -1 });

module.exports = mongoose.model("PerfResult", PerfResultSchema);
