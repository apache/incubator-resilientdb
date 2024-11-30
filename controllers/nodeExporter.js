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
async function getCpuUsage(req,res){
    const { query, from = "now-5m" , until = "now" ,step = 28 } = req.body
    logger.info(req.body)
    const baseUrl = buildUrl(getEnv("NODE_EXPORTER_BASE_URL"),{
        query,
        start:from,
        end:until,
        step
    })
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
      };
      logger.info(config)
      try {
        const response = await axios.request(config) 
        return res.send(response.data)
      } catch (error) {
        logger.error(error)
        return res.status(400).send({
            error: error
        })
      }
}

module.exports = {
    getCpuUsage
}