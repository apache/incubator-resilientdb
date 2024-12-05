const axios = require('axios');
const { buildUrl } = require('../utils/urlHelper');
const { getEnv } = require('../utils/envParser');
const logger = require('../utils/logger');
async function getCpuUsage(req, res) {
    const { query, from = "now-5m", until = "now", step = 28 } = req.body;
    logger.info(req.body);
    const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
        query,
        start: from,
        end: until,
        step
    });
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };
    logger.info(config);
    try {
        const response = await axios.request(config);
        let data = response?.data?.data?.result.filter((val) => val?.metric?.groupname === 'kv_service')
        let responseData = []
        if(data.length > 0){
          responseData = data[0]?.values
        }    
        return res.send({data : responseData});
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred"
        });
    }
}
//// Disk IOPS : 
async function getDiskIOPS(req, res) {
    const { query, from = "now-5m", until = "now", step = 28 } = req.body;
    logger.info(req.body);
    const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
        query,
        start: from,
        end: until,
        step
    });
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };
    logger.info(config);
    try {
        const response = await axios.request(config);
        // Access the 'result' array and filter based on 'device' and 'job'
        let data = response?.data?.data?.result.filter((val) => 
            val?.metric?.device === 'sda' && val?.metric?.job === 'node_exporter'
        );
        // If we have data, extract the 'values' from the first matching entry
        let responseData = [];
        if (data.length > 0) {
            responseData = data[0]?.values;
        }
        // Send the filtered values in the response
        return res.send({ data: responseData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred"
        });
    }
}
/// Disk Wait time : 
async function getDiskWaitTime(req, res) {
    const { from = "now-5m", until = "now", step = 28 } = req.body;

    try {
        // Define queries for both metrics
        const queries = {
            readWaitTime: "rate(node_disk_read_time_seconds_total{device='sda'}[5m]) / rate(node_disk_reads_completed_total{device='sda'}[5m])",
            writeWaitTime: "rate(node_disk_write_time_seconds_total{device='sda'}[5m]) / rate(node_disk_writes_completed_total{device='sda'}[5m])",
        };

        // Execute Prometheus queries in parallel
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

        // Format the responses for each metric
        const formattedData = responses.reduce((acc, { key, values }) => {
            acc[key] = values.map(([timestamp, value]) => ({
                time: new Date(timestamp * 1000).toISOString(),
                value: parseFloat(value),
            }));
            return acc;
        }, {});

        // Send formatted data to the frontend
        return res.send({ data: formattedData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred while fetching Disk Wait Time data",
        });
    }
}

// Time spent doing IO

async function getTimeSpentDoingIO(req, res) {
    const { from = "now-5m", until = "now", step = 28 } = req.body;

    const query = "rate(node_disk_io_time_seconds_total{device='sda', job='node_exporter'}[5m])";

    try {
        // Build the Prometheus query URL
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

        // Fetch data from Prometheus
        const response = await axios.request(config);

        // Parse response
        const result = response?.data?.data?.result?.[0]?.values || [];

        // Format data for the frontend
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
 * Processes Node Exporter data to find a specific groupname and simplify the structure.
 * 
 * @param {Object} data - The raw data from Node Exporter.
 * @param {string} groupname - The groupname to filter for.
 * @returns {Object} - The filtered and simplified data.
 */
module.exports = {
    getCpuUsage,
    getDiskIOPS,
    getDiskWaitTime,
    getTimeSpentDoingIO
};
