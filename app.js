const express = require("express");
const healthRoutes = require("./routes/healthcheck");
const pyroscopeRoutes = require("./routes/pyroscope")
const nodeExporterRoutes = require("./routes/nodeExporter")
const statsExporterRoutes = require("./routes/statsExporter")
const transactionsRoutes = require("./routes/transactions")
const explorerRoutes = require("./routes/explorer")
const cors = require("cors")

const app = express();

app.use(cors());

app.use(express.json());

// Health check route
app.use("/api/v1", healthRoutes);
app.use("/api/v1/pyroscope",pyroscopeRoutes)
app.use("/api/v1/nodeExporter",nodeExporterRoutes)
app.use("/api/v1/statsExporter",statsExporterRoutes)
app.use("/api/v1/transactions",transactionsRoutes)
app.use("/api/v1/explorer",explorerRoutes)

module.exports = app;
