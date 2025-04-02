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

/**
 * @typedef {Object} Transaction
 * @property {string} cmd - The command executed in the transaction (e.g., "SET")
 * @property {string} key - The key being operated on
 * @property {string} value - The value associated with the key
 */

/**
 * @typedef {Object} Block
 * @property {number} id - The unique identifier of the block
 * @property {string} number - The block number as a string
 * @property {Transaction[]} transactions - Array of transactions contained in the block
 * @property {number} size - The size of the block in bytes
 * @property {string} createdAt - The creation timestamp in human-readable format (GMT)
 * @property {number} createdAtEpoch - The creation timestamp as Unix epoch in microseconds
 */

const axios = require('axios');
const { getEnv } = require('../utils/envParser');
const logger = require('../utils/logger');
const  { parseCreateTime, parseTimeToUnixEpoch} = require('../utils/time');
const { applyDeltaEncoding, decodeDeltaEncoding } = require('../utils/encoding');

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
 * @param {Object} req.query - The query parameters.
 * @param {number} req.query.start - The starting block number.
 * @param {number} req.query.end - The ending block number.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the fetched data or an error response.
 */
async function getBlocks(req, res) {
    const start = parseInt(req.query.start, 10);
    const end = parseInt(req.query.end, 10);
    let baseUrl = `${getEnv("EXPLORER_BASE_URL")}/v1/blocks`

    if (isNaN(start) || isNaN(end)) {
        return res.status(400).send({
            error: 'Pass valid start and end query params as part of the request',
        });
    }

    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        /** @type {Array<Block>} */
        const data = response?.data
        return res.send(data);
    } catch (error) {
        logger.error('Error fetching block data:', error);
        return res.status(500).send({
            error: 'Failed to fetch block data',
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
 * @param {Object} req.query - The query parameters.
 * @param {number} req.query.start - The starting block number.
 * @param {number} req.query.end - The ending block number.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the fetched data or an error response.
 */
async function getEncodedBlocks(req, res) {
    const start = parseInt(req.query.start, 10);
    const end = parseInt(req.query.end, 10);
    let baseUrl = `${getEnv("EXPLORER_BASE_URL")}/v1/blocks`

    if (!isNaN(start) && !isNaN(end)) {
        baseUrl = `${getEnv("EXPLORER_BASE_URL")}/v1/blocks/${start}/${end}`;
    }

    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        /** @type {Array<Block>} */
        const data = response?.data
        let modifiedData = data.map((record)=>{
            return {
                epoch: parseTimeToUnixEpoch(record?.createdAt),
                volume: record?.transactions?.length || 0
            }
        })
        modifiedData = applyDeltaEncoding(modifiedData)
        return res.send(modifiedData);
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
    getEncodedBlocks
};
