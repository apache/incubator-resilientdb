const express = require("express");
const router = express.Router();

/**
 * Health check route
 * @route GET /healthcheck
 * @returns {Object} 200 - Health status
 */
router.get("/healthcheck", (req, res) => {
  res.status(200).json({ status: "UP", timestamp: new Date().toISOString() });
});

module.exports = router;
