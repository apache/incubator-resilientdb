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
 */

const fs = require('fs-extra');
const path = require('path');
const os = require('os');
const yaml = require('js-yaml');  
const logger = require('./logger');

async function getResDBHome() {
  try {
    if (process.env.ResDB_Home) {
      return process.env.ResDB_Home;
    }

    let configFilePath = path.join(process.cwd(), 'config.yaml');
    if (!(await fs.pathExists(configFilePath))) {
      configFilePath = path.join(os.homedir(), 'config.yaml');
      if (!(await fs.pathExists(configFilePath))) {
        logger.error('ResDB_Home is not set and config.yaml file not found.');
        throw new Error('ResDB_Home is not set and config.yaml file not found. Please set ResDB_Home environment variable or provide a config.yaml file.');
      }
    }

    const fileContents = await fs.readFile(configFilePath, 'utf8');
    const config = yaml.load(fileContents);

    if (config && config.ResDB_Home) {
      process.env.ResDB_Home = config.ResDB_Home;
      return config.ResDB_Home;
    } else {
      logger.error('ResDB_Home is not defined in config.yaml.');
      throw new Error('ResDB_Home is not defined in config.yaml. Please define ResDB_Home in your config.yaml file.');
    }
  } catch (error) {
    logger.error(`Error accessing ResDB_Home: ${error.message}`);
    throw error;
  }
}

module.exports = {
  getResDBHome
};
