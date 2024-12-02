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

/**
 * Processes Node Exporter data to find a specific groupname and simplify the structure.
 * 
 * @param {Object} data - The raw data from Node Exporter.
 * @param {string} groupname - The groupname to filter for.
 * @returns {Object} - The filtered and simplified data.
 */
module.exports = {
    getCpuUsage
};
