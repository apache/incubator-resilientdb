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
 * @typedef {Object} Env
 * @property {string} [PORT] - The port the application runs on.
 * @property {string} [PYROSCOPE_BASE_URL] - The base URL for the Pyroscope service.
 * @property {string} [NODE_EXPORTER_BASE_URL] - The base URL for the Node exporter service.
 * @property {string} [PROCESS_EXPORTER_BASE_URL] - The base URL for the Process exporter service.
 * @property {string} [EXPLORER_BASE_URL] - The base URL for the Explorer Service
 * @property {string} [CPP_STATS_API_BASE_URL] - The base URL for the CPP stats service.
 * @property {string} [CPP_TRANSACTIONS_API_BASE_URL] - The base URL for the CPP Graphql service.
 */

/**
 * Retrieves an environment variable by name, ensuring it's definsed if marked as mandatory.
 *
 * @param {keyof Env} name - The name of the environment variable to retrieve.
 * @param {any} [optional] - Default Value if env does not exist. throws an error if env is not defined and optional is not passed
 * @returns {string|undefined} The value of the environment variable if defined.
 * @throws {Error} If the environment variable is required and not defined.
 */
function getEnv(name, optional = null) {
    const value = process.env[name] || optional;
    
    if (value === undefined || value === null || value === "") {
      throw new Error(`Environment variable "${name}" is required but not defined.`);
    }
  
    return value;
  };


module.exports = {
    getEnv
}