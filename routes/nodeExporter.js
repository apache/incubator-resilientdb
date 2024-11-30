const express = require("express");
const { getCpuUsage } = require("../controllers/nodeExporter");
const router = express.Router();

/**
 * Health check route
 * @route GET /getCpuUsage
 * @returns {Object} 200 - cpuUsage data
 */
router.post("/getCpuUsage", getCpuUsage);

module.exports = router;
