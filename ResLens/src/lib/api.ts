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

import axios from 'axios';

const createApiInstance = (baseURL: string) => {
    return axios.create({
        baseURL,
    });
};

// Default to localhost:3003 if env var is not set
const middlewareBaseUrl = import.meta.env.VITE_MIDDLEWARE_BASE_URL || "http://localhost:3003/api/v1";
const middlewareSecondaryBaseUrl = import.meta.env.VITE_MIDDLEWARE_SECONDARY_BASE_URL || "http://localhost:3003/api/v1";

console.log("Middleware Base URL:", middlewareBaseUrl);
console.log("Middleware Secondary Base URL:", middlewareSecondaryBaseUrl);

const middlewareApi = createApiInstance(middlewareBaseUrl);
const middlewareSecondaryApi = createApiInstance(middlewareSecondaryBaseUrl);

export {
    middlewareApi, 
    middlewareSecondaryApi,
}
