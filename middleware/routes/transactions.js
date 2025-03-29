const express = require("express");
const { setValue, getValue, calculateP99 } = require("../middleware/controllers/transactions")
const router = express.Router();

/**
 * Health check route
 * @route POST /set
 * @returns {Object} 200 - transaction data
 */
router.post("/set", setValue);
router.get("/get/:key", getValue);
router.post("/calculateP99", calculateP99);

module.exports = router;