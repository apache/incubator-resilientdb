const express = require("express");
const { getProfilingData } = require("../controllers/pyroscope");
const router = express.Router();

/**
 * Health check route
 * @route GET /getProfile
 * @returns {Object} 200 - proflingData
 */
router.post("/getProfile", getProfilingData);

module.exports = router;
