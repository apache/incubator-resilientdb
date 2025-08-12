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
const { buildUrl } = require('../utils/urlHelper');
const { getEnv } = require('../utils/envParser');
const logger = require('../utils/logger');

/**
 * Retrieves CPU profiling data from Pyroscope and sends it as a response.
 *
 * @async
 * @function cpuUsage
 * @param {import('express').Request} req - The Express request object.
 * @param {Object} req.body - The body of the request containing query parameters.
 * @param {string} req.body.query - The query parameter to filter the profiling data (without the `.cpu` suffix).
 * @param {string} [req.body.from="now-5m"] - The start time for the profiling data range (default: 5 minutes ago).
 * @param {string} [req.body.until="now"] - The end time for the profiling data range (default: now).
 * @param {string} [req.body.format="json"] - The format of the profiling data (default: JSON).
 * @param {import('express').Response} res - The Express response object.
 * @returns {Promise<void>} Sends the profiling data as a response or an error response.
 */
async function cpuUsage(req, res) {
    const { query, from = "now-5m", until = "now", format = "json" } = req.body;
    const baseUrl = buildUrl(getEnv("PYROSCOPE_BASE_URL"), {
        query: `${query}.cpu`,
        from: from,
        until: until,
        format: format,
    });

    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        return res.send(response.data);
    } catch (error) {
        logger.error(error);
        return res.status(500).send({
            error: error.message || "An error occurred while fetching CPU profiling data",
        });
    }
}

module.exports = {
    cpuUsage,
};