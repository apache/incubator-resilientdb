const express = require("express");
const { getExplorerData, getBlocks } = require("../controllers/explorer");
const router = express.Router();

/**
 * @route GET /getExplorerData
 * @returns {Object} 200 - explorer data
 */
router.get("/getExplorerData", getExplorerData);
router.get("/getBlocks", getBlocks);

module.exports = router;