const axios = require('axios');
const logger = require('../utils/logger');

async function getExplorerData(req,res){
    const baseUrl = `http://localhost:18000/populatetable`
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
      };
      logger.info(config)
      try {
        let response = await axios.request(config) 
        return res.send(response.data)
      } catch (error) {
        logger.error(error)
        return res.status(500).send({
            error: error
        })
      }
}

async function getBlocks(req,res){
    const baseUrl = `http://localhost:18000/v1/blocks`
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
      };
      logger.info(config)
      try {
        let response = await axios.request(config) 
        return res.send(response.data)
      } catch (error) {
        logger.error(error)
        return res.status(500).send({
            error: error
        })
      }
}

module.exports = {
    getExplorerData,
    getBlocks
}