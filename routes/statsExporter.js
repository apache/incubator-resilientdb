const express = require("express");
const { getTransactionData} = require("../controllers/statsExporter");
const router = express.Router();

/**
 * Health check route
 * @route GET /getTransactionData
 * @returns {Object} 200 - transaction data
 */
router.post("/getTransactionData", getTransactionData);

module.exports = router;