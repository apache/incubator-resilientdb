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
* software distributed under this work for additional information
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

const logger = require("../utils/logger");

// In-memory store for query statistics (in production, use a database)
const queryStatsStore = [];

/**
 * Store GraphQL query statistics
 * @route POST /api/v1/queryStats/store
 * @param {Object} req.body - Query statistics data
 * @param {string} req.body.query - The GraphQL query
 * @param {Object} req.body.efficiency - Efficiency metrics
 * @param {Object} req.body.explanation - Query explanation
 * @param {Array} req.body.optimizations - Optimization suggestions
 * @returns {Object} 200 - Success response
 */
const storeQueryStats = async (req, res) => {
  try {
    const { query, efficiency, explanation, optimizations, timestamp } = req.body;

    if (!query) {
      return res.status(400).json({
        error: "Query is required",
      });
    }

    // Ensure complexity is set in efficiency object
    const efficiencyWithComplexity = efficiency || {};
    if (!efficiencyWithComplexity.complexity) {
      // Try to extract from explanation if available
      efficiencyWithComplexity.complexity = explanation?.complexity || "medium";
    }
    // Normalize complexity to lowercase
    if (efficiencyWithComplexity.complexity) {
      efficiencyWithComplexity.complexity = efficiencyWithComplexity.complexity.toLowerCase();
    }

    const statsEntry = {
      id: `query_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
      query,
      efficiency: efficiencyWithComplexity,
      explanation: explanation || {},
      optimizations: optimizations || [],
      timestamp: timestamp || new Date().toISOString(),
      createdAt: new Date().toISOString(),
    };

    queryStatsStore.push(statsEntry);

    logger.info(`Stored query statistics for query: ${query.substring(0, 50)}...`);

    res.status(200).json({
      success: true,
      id: statsEntry.id,
      message: "Query statistics stored successfully",
    });
  } catch (error) {
    logger.error("Error storing query statistics:", error);
    res.status(500).json({
      error: "Failed to store query statistics",
      message: error.message,
    });
  }
};

/**
 * Get query statistics
 * @route GET /api/v1/queryStats
 * @param {Object} req.query - Query parameters
 * @param {number} req.query.limit - Number of results to return (default: 10)
 * @returns {Object} 200 - Query statistics
 */
const getQueryStats = async (req, res) => {
  try {
    const limit = parseInt(req.query.limit, 10) || 10;
    const stats = queryStatsStore.slice(-limit).reverse();

    res.status(200).json({
      success: true,
      count: stats.length,
      total: queryStatsStore.length,
      data: stats,
    });
  } catch (error) {
    logger.error("Error retrieving query statistics:", error);
    res.status(500).json({
      error: "Failed to retrieve query statistics",
      message: error.message,
    });
  }
};

/**
 * Get statistics for a specific query
 * @route GET /api/v1/queryStats/:id
 * @param {string} req.params.id - Query statistics ID
 * @returns {Object} 200 - Query statistics
 */
const getQueryStatById = async (req, res) => {
  try {
    const { id } = req.params;
    const stats = queryStatsStore.find((entry) => entry.id === id);

    if (!stats) {
      return res.status(404).json({
        error: "Query statistics not found",
      });
    }

    res.status(200).json({
      success: true,
      data: stats,
    });
  } catch (error) {
    logger.error("Error retrieving query statistics:", error);
    res.status(500).json({
      error: "Failed to retrieve query statistics",
      message: error.message,
    });
  }
};

/**
 * Get aggregated statistics
 * @route GET /api/v1/queryStats/aggregated
 * @returns {Object} 200 - Aggregated statistics
 */
const getAggregatedStats = async (req, res) => {
  try {
    const totalQueries = queryStatsStore.length;
    const avgEfficiency = queryStatsStore.reduce((sum, entry) => {
      return sum + (entry.efficiency?.score || 0);
    }, 0) / (totalQueries || 1);

    const complexityDistribution = queryStatsStore.reduce((acc, entry) => {
      // Try multiple sources for complexity
      let complexity = entry.efficiency?.complexity || 
                      entry.explanation?.complexity || 
                      "unknown";
      // Normalize to lowercase
      complexity = complexity.toLowerCase();
      // Map common variations
      if (complexity === "high" || complexity === "complex") {
        complexity = "high";
      } else if (complexity === "medium" || complexity === "moderate") {
        complexity = "medium";
      } else if (complexity === "low" || complexity === "simple") {
        complexity = "low";
      } else {
        complexity = "unknown";
      }
      acc[complexity] = (acc[complexity] || 0) + 1;
      return acc;
    }, {});

    res.status(200).json({
      success: true,
      data: {
        totalQueries,
        avgEfficiency: Math.round(avgEfficiency * 100) / 100,
        complexityDistribution,
        recentQueries: queryStatsStore.slice(-5).reverse(),
      },
    });
  } catch (error) {
    logger.error("Error retrieving aggregated statistics:", error);
    res.status(500).json({
      error: "Failed to retrieve aggregated statistics",
      message: error.message,
    });
  }
};

module.exports = {
  storeQueryStats,
  getQueryStats,
  getQueryStatById,
  getAggregatedStats,
};

