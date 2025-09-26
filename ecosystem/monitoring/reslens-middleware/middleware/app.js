/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*/

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
