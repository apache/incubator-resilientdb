const express = require("express");
const { getCpuUsage, getDiskIOPS, getDiskWaitTime, getTimeSpentDoingIO, getDiskRWData } = require("../middleware/controllers/nodeExporter");
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
router.post("/getDiskRWData", getDiskRWData);

module.exports = router;
