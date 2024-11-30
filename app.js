const express = require("express");
const healthRoutes = require("./routes/healthcheck");
const pyroscopeRoutes = require("./routes/pyroscope")
const nodeExporterRoutes = require("./routes/nodeExporter")
const cors = require("cors")

const app = express();

app.use(cors());

app.use(express.json());

// Health check route
app.use("/", healthRoutes);
app.use("/api/v1/pyroscope",pyroscopeRoutes)
app.use("/api/v1/nodeExporter",nodeExporterRoutes)

module.exports = app;
