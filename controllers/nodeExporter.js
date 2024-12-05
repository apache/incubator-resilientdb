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

    const queries = [
        "rate(node_disk_read_time_seconds_total{device='sda'}[5m]) / rate(node_disk_reads_completed_total{device='sda'}[5m])",
        "rate(node_disk_write_time_seconds_total{device='sda'}[5m]) / rate(node_disk_writes_completed_total{device='sda'}[5m])",
    ];

    try {
        // Fetch data for both queries
        const responses = await Promise.all(
            queries.map((query) => {
                const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
                    query,
                    start: from,
                    end: until,
                    step,
                });
                return axios.get(baseUrl);
            })
        );

        // Parse responses for "read" and "write" metrics
        const readWaitTime = responses[0]?.data?.data?.result?.[0]?.values || [];
        const writeWaitTime = responses[1]?.data?.data?.result?.[0]?.values || [];

        // Format the data for plotting
        const formattedData = {
            readWaitTime: readWaitTime.map(([timestamp, value]) => ({
                time: new Date(timestamp * 1000).toISOString(),
                value: parseFloat(value),
            })),
            writeWaitTime: writeWaitTime.map(([timestamp, value]) => ({
                time: new Date(timestamp * 1000).toISOString(),
                value: parseFloat(value),
            })),
        };

        // Send the formatted data
        return res.send({ data: formattedData });
    } catch (error) {
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred",
        });
    }
}
// Time spent doing IO

async function getTimeSpentDoingIO(req, res) {
    const { query, from = "now-5m", until = "now", step = 28 } = req.body;
    logger.info(req.body);

    // Build the Prometheus query URL
    const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"), {
        query,
        start: from,
        end: until,
        step
    });

    const config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
    };

    logger.info(config);

    try {
        // Make the API request to Prometheus
        const response = await axios.request(config);

        // Access the 'result' array and filter by 'device' and 'job'
        const data = response?.data?.data?.result.filter(
            (val) => val?.metric?.device === 'sda' && val?.metric?.job === 'node_exporter'
        );

        // Extract 'values' from the filtered data
        let responseData = [];
        if (data && data.length > 0) {
            responseData = data[0]?.values.map(([timestamp, value]) => ({
                timestamp: Number(timestamp), // Convert timestamp to number
                value: parseFloat(value),     // Convert value to number
            }));
        }

        // Send the formatted data in the response
        return res.send({ data: responseData });
    } catch (error) {
        // Log the error and send a failure response
        logger.error(error);
        return res.status(400).send({
            error: error.message || "An error occurred while fetching Disk IO Time data",
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
