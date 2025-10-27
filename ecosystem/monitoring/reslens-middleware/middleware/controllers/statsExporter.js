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
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*/

const axios = require('axios');
const { getEnv } = require('../utils/envParser');
const logger = require('../utils/logger');

/**
 * Represents the statistics for a specific LevelDB level.
 * @typedef {Object} LevelStats
 * @property {string} Level - The level number in LevelDB.
 * @property {string} Files - The number of files in the level.
 * @property {string} SizeMB - The size of the level in MB.
 * @property {string} TimeSec - The time spent on compactions in seconds.
 * @property {string} ReadMB - The amount of data read during compactions in MB.
 * @property {string} WriteMB - The amount of data written during compactions in MB.
 */

/**
 * Represents the full set of data for LevelDB monitoring and statistics.
 * @typedef {Object} LevelDbData
 * @property {number} ext_cache_hit_ratio - External cache hit ratio.
 * @property {string} level_db_approx_mem_size - Approximate memory size in bytes.
 * @property {LevelStats[]} level_db_stats - Array of level statistics.
 * @property {number} max_resident_set_size - Maximum resident set size in KB.
 * @property {number} resident_set_size - Current resident set size in KB.
 */

/**
 * Retrieves profiling data and sends it as a response.
 *
 * @param {import('express').Request} req - The Express request object.
 * @param {import('express').Response} res - The Express response object.
 * @param {Object} req.body - The body of the request.
 * @param {string} req.body.query - The query parameter to filter the profiling data. Send it without .cpu prefix
 * @returns {void} - Sends the profiling data as a response.
 */
async function getTransactionData(req,res){
    const baseUrl = `${getEnv("CPP_STATS_API_BASE_URL")}/transaction_data`
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
      };
      logger.info(config)
      try {
        let response = await axios.request(config) 
        const statsData = response?.data?.level_db_stats;
        response.data.level_db_stats = processStatsData(statsData);
        return res.send(response.data)
      } catch (error) {
        logger.error(error)
        return res.status(500).send({
            error: error
        })
      }
}

/**
 * @param {string} statsData 
 * @return {Array<LevelStats>}
 */
function processStatsData(statsData){
  if(!statsData){
    return []
  }
  const lines = statsData.trim().split('\n');
  const headers = lines[1].trim().split(/\s+/);
  const rows = lines.slice(3);
  const parsedStatsData = rows.map(row => {
     const values = row.trim().split(/\s+/);
      return headers.reduce((acc, header, index) => {
      acc[header] = values[index];
      return acc;
  }, {});
  });
  return parsedStatsData
}

module.exports = {
    getTransactionData
}