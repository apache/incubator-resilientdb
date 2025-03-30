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
 * Retrieves profiling data and sends it as a response.
 *
 * @param {import('express').Request} req - The Express request object.
 * @param {import('express').Response} res - The Express response object.
 * @param {Object} req.body - The body of the request.
 * @param {string} req.body.query - The query parameter to filter the profiling data. Send it without .cpu prefix
 * @returns {void} - Sends the profiling data as a response.
 */
async function setValue(req,res){
    const baseUrl = `${getEnv("CPP_TRANSACTIONS_API_BASE_URL")}/commit`
    const data = req.body
    let config = {
        method: 'post',
        maxBodyLength: Infinity,
        url: baseUrl,
        data : data
      };
      try {
        let response = await axios.request(config) 
        const responseData = response?.data.split(": ")
        return res.send({
            id: responseData[1]
        })
      } catch (error) {
        logger.error(error)
        return res.status(500).send({
            error: error
        })
      }
}

/**
 * Retrieves profiling data and sends it as a response.
 *
 * @param {import('express').Request} req - The Express request object.
 * @param {import('express').Response} res - The Express response object.
 * @param {Object} req.body - The body of the request.
 * @param {string} req.body.query - The query parameter to filter the profiling data. Send it without .cpu prefix
 * @returns {void} - Sends the profiling data as a response.
 */
async function getValue(req,res){
    const { key } = req.params
    const baseUrl = `${getEnv("CPP_TRANSACTIONS_API_BASE_URL")}/${key}`
    const data = req.body
    let config = {
        method: 'get',
        maxBodyLength: Infinity,
        url: baseUrl,
      };
      try {
        let response = await axios.request(config) 
        return res.send(response?.data)
      } catch (error) {
        logger.error(error)
        return res.status(500).send({
            error: error
        })
      }
}

/**
 * Warms the cache by inserting 50 random key-value pairs.
 *
 * @param {import('express').Request} req - The Express request object.
 * @param {import('express').Response} res - The Express response object.
 * @returns {void} - Sends a success message or an error as a response.
 */
async function calculateP99(req, res) {
  const { samples = 50 } = req.body
  const baseUrl = `${getEnv("CPP_TRANSACTIONS_API_BASE_URL")}/commit`;

  let promises = [];
  let responseTimes = [];

  for (let i = 0; i < Math.min(50,samples); i++) {
    const randomValue = getRandomInt(samples)
    const key = `key_${randomValue}`;
    const value = `value_${randomValue}`;

    const data = {
      id: key,
      value: value,
    };

    const config = {
      method: "post",
      maxBodyLength: Infinity,
      url: baseUrl,
      data: data,
    };

    promises.push(
      new Promise(async (resolve, reject) => {
        const startTime = Date.now();
        try {
          const response = await axios.request(config);
          const responseTime = Date.now() - startTime;
          responseTimes.push(responseTime);

          logger.info(`Key: ${key} set successfully. Response time: ${responseTime}ms`);
          resolve(response.data);
        } catch (error) {
          logger.error(`Failed to set key: ${key}. Error: ${error.message}`);
          reject(error);
        }
      })
    );
  }

  try {
    await Promise.all(promises);

    responseTimes.sort((a, b) => a - b);
    const p99Index = Math.ceil(0.99 * responseTimes.length) - 1;
    const p99 = responseTimes[p99Index];

    return res.send({
      message: `Cache warmed with ${samples} random values successfully.`,
      p99,
    });
  } catch (error) {
    logger.error("Error warming the cache:", error.message);
    return res.status(500).send({
      error: "Failed to warm cache.",
    });
  }
}

function getRandomInt(max) {
  return Math.floor(Math.random() * max);
}

module.exports = {
    setValue,
    getValue,
    calculateP99
}