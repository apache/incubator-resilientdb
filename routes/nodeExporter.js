const express = require("express");
const { getCpuUsage, getDiskIOPS, getDiskWaitTime, getTimeSpentDoingIO } = require("../controllers/nodeExporter");
const router = express.Router();

/**
 * Health check route
 * @route GET /getCpuUsage
 * @returns {Object} 200 - cpuUsage data
 */
router.post("/getCpuUsage", getCpuUsage);
router.post("/getDiskIOPS", getDiskIOPS);
router.post("/getDiskWaitTime", getDiskWaitTime);
router.post("/getTimeSpentDoingIO", getTimeSpentDoingIO);

module.exports = router;
