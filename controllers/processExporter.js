const axios = require('axios');
const { buildUrl } = require('../utils/urlHelper');
const { getEnv } = require('../utils/envParser');
const logger = require('../utils/logger');

/**
 * Retrieves profiling data and sends it as a response.
 *
 * @param {import('express').Request} req - The Express request object.
 * @param {import('express').Response} res - The Express response object.
 * @param {Object} req.body - The body of the request.
 * @param {string} req.body.query - The query parameter to filter the profiling data. Send it without .cpu prefix
 * @returns {void} - Sends the profiling data as a response.
 */
async function cpuUsage(req,res){
    const { query, from = "now-5m" , until = "now" ,format = "json" } = req.body
    const baseUrl = buildUrl(getEnv("PYROSCOPE_BASE_URL"),{
        query: `${query}.cpu`,
        from: from,
        until: until,
        format: format
    })
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
      };
      try {
        const response = await axios.request(config) 
        return res.send(response.data)
      } catch (error) {
        return res.send({
            error: error
        })
      }
}

module.exports = {
    getProfilingData
}