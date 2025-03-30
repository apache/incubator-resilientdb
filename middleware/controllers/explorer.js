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
 * Fetches explorer data from the EXPLORER_BASE_URL and sends it as a response.
 *
 * @async
 * @function getExplorerData
 * @param {Object} req - The HTTP request object.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the fetched data or an error response.
 */
async function getExplorerData(req, res) {
    const baseUrl = `${getEnv("EXPLORER_BASE_URL")}/populatetable`;
    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        return res.send(response.data);
    } catch (error) {
        logger.error('Error fetching explorer data:', error);
        return res.status(500).send({
            error: 'Failed to fetch explorer data',
            details: error.message,
        });
    }
}

/**
 * Fetches block data from the EXPLORER_BASE_URL and sends it as a response.
 *
 * @async
 * @function getBlocks
 * @param {Object} req - The HTTP request object.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the fetched data or an error response.
 */
async function getBlocks(req, res) {
    const baseUrl = `${getEnv("EXPLORER_BASE_URL")}/v1/blocks`;
    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        return res.send(response.data);
    } catch (error) {
        logger.error('Error fetching block data:', error);
        return res.status(500).send({
            error: 'Failed to fetch block data',
            details: error.message,
        });
    }
}

module.exports = {
    getExplorerData,
    getBlocks,
};