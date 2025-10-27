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
 * Fetches CPU usage data from Node Exporter and sends it as a response.
 *
 * @async
 * @function getCpuUsage
 * @param {Object} req - The HTTP request object.
 * @param {Object} req.body - The request body containing query parameters.
 * @param {string} req.body.query - The PromQL query to fetch CPU usage data.
 * @param {string} [req.body.from="now-5m"] - The start time for the query range (default: 5 minutes ago).
 * @param {string} [req.body.until="now"] - The end time for the query range (default: now).
 * @param {number} [req.body.step=28] - The step interval in seconds for the query (default: 28 seconds).
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the filtered CPU usage data or an error response.
 */
async function getCpuUsage(req, res) {
    const { query, from = "now-5m", until = "now", step = 28 } = req.body;
    logger.debug(req.body);
    const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
        query,
        start: from,
        end: until,
        step,
    });

    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        const data = response?.data?.data?.result.filter((val) => val?.metric?.groupname === 'kv_service');
        const responseData = data.length > 0 ? data[0]?.values : [];
        return res.send({ data: responseData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred",
        });
    }
}

/**
 * Fetches Disk IOPS (Input/Output Operations Per Second) data from Node Exporter.
 *
 * @async
 * @function getDiskIOPS
 * @param {Object} req - The HTTP request object containing query parameters in the body.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the formatted Disk IOPS data or an error response.
 */
async function getDiskIOPS(req, res) {
    const { from = "now-12h", until = "now", step = 28 } = req.body;

    const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
        query: "rate(node_disk_writes_completed_total{device='vda',job='node_exporter'}[5m])",
        start: from,
        end: until,
        step,
    });

    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    try {
        const response = await axios.request(config);
        const data = response?.data?.data?.result.filter(
            (val) => val?.metric?.device === "vda" && val?.metric?.job === "node_exporter"
        );

        const formattedResponseData = data.length > 0
            ? data[0]?.values.map(([timestamp, value]) => ({
                time: new Date(Number(timestamp) * 1000).toISOString(),
                value: parseFloat(value).toFixed(2),
            }))
            : [];

        return res.send({ data: formattedResponseData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred while fetching the DiskIOPS data",
        });
    }
}

/**
 * Fetches Disk Wait Time data (read and write) from Node Exporter.
 *
 * @async
 * @function getDiskWaitTime
 * @param {Object} req - The HTTP request object containing query parameters in the body.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the formatted Disk Wait Time data or an error response.
 */
async function getDiskWaitTime(req, res) {
    const { from = "now-5m", until = "now", step = 28 } = req.body;

    try {
        const queries = {
            readWaitTime: "rate(node_disk_read_time_seconds_total{device='vda'}[5m]) / rate(node_disk_reads_completed_total{device='vda'}[5m])",
            writeWaitTime: "rate(node_disk_write_time_seconds_total{device='vda'}[5m]) / rate(node_disk_writes_completed_total{device='vda'}[5m])",
        };

        const responses = await Promise.all(
            Object.entries(queries).map(async ([key, query]) => {
                const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
                    query,
                    start: from,
                    end: until,
                    step,
                });
                const response = await axios.get(baseUrl);
                return {
                    key,
                    values: response?.data?.data?.result?.[0]?.values || [],
                };
            })
        );

        const formattedData = responses.reduce((acc, { key, values }) => {
            acc[key] = values.map(([timestamp, value]) => ({
                time: new Date(timestamp * 1000).toISOString(),
                value: parseFloat(value),
            }));
            return acc;
        }, {});

        return res.send({ data: formattedData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred while fetching Disk Wait Time data",
        });
    }
}

/**
 * Fetches the time spent doing I/O operations from Node Exporter.
 *
 * @async
 * @function getTimeSpentDoingIO
 * @param {Object} req - The HTTP request object containing query parameters in the body.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the formatted I/O time data or an error response.
 */
async function getTimeSpentDoingIO(req, res) {
    const { from = "now-5m", until = "now", step = 28 } = req.body;

    const query = "rate(node_disk_io_time_seconds_total{device='vda', job='node_exporter'}[5m])";

    try {
        const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
            query,
            start: from,
            end: until,
            step,
        });

        const config = {
            method: 'get',
            url: baseUrl,
        };

        const response = await axios.request(config);
        const result = response?.data?.data?.result?.[0]?.values || [];

        const formattedData = result.map(([timestamp, value]) => ({
            time: new Date(timestamp * 1000).toISOString(),
            value: parseFloat(value),
        }));

        return res.send({ data: formattedData });
    } catch (error) {
        logger.error("Error fetching Disk IO Time data:", error);
        return res.status(400).send({
            error: error.message || "Failed to fetch Disk IO Time data",
        });
    }
}

/**
 * Fetches Disk Read and Write Merged data from Node Exporter.
 *
 * @async
 * @function getDiskRWData
 * @param {Object} req - The HTTP request object containing query parameters in the body.
 * @param {Object} res - The HTTP response object.
 * @returns {Promise<void>} Sends the formatted Disk Read and Write Merged data or an error response.
 */
async function getDiskRWData(req, res) {
    const { from = "now-5m", until = "now", step = 28 } = req.body;

    try {
        const queries = {
            readMerged: "rate(node_disk_reads_merged_total{device='vda'}[5m])",
            writeMerged: "rate(node_disk_writes_merged_total{device='vda'}[5m])",
        };

        const responses = await Promise.all(
            Object.entries(queries).map(async ([key, query]) => {
                const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
                    query,
                    start: from,
                    end: until,
                    step,
                });
                const response = await axios.get(baseUrl);
                return {
                    key,
                    values: response?.data?.data?.result?.[0]?.values || [],
                };
            })
        );

        const formattedData = responses.reduce((acc, { key, values }) => {
            acc[key] = values.map(([timestamp, value]) => ({
                time: new Date(timestamp * 1000).toISOString(),
                value: parseFloat(value),
            }));
            return acc;
        }, {});

        return res.send({ data: formattedData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred while fetching Disk Merged Data",
        });
    }
}

module.exports = {
    getCpuUsage,
    getDiskIOPS,
    getDiskWaitTime,
    getTimeSpentDoingIO,
    getDiskRWData,
};