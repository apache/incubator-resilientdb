const express = require("express");
const { getProfilingData } = require("../controllers/pyroscope");
const router = express.Router();

/**
 * Health check route
 * @route GET /healthcheck
 * @returns {Object} 200 - Health status
 */
router.post("/getProfile", getProfilingData);

module.exports = router;
