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
 * @typedef {Object} TimeSeriesPoint
 * @property {number} epoch - Unix timestamp in microseconds
 * @property {string} createdAt - GMT timestamp
 * @property {number} volume - Number of transactions
 */

/**
 * @typedef {Object} DeltaEncodedData
 * @property {number} startEpoch - Base epoch value (first timestamp, divided by timeMultiplier)
 * @property {number} startVolume - Base volume value (first volume)
 * @property {number} timeMultiplier - Factor by which epoch values were divided
 * @property {number[]} epochs - Array of delta values for epochs (normalized)
 * @property {number[]} volumes - Array of delta values for volumes
 */

const { parseCreateTime } = require("./time");

// Define a constant for time normalization
const TIME_MULTIPLIER = 1000000;

/**
 * Applies delta encoding to a time series array
 * 
 * @param {Array<TimeSeriesPoint>} data - Original time series data
 * @returns {DeltaEncodedData} Delta-encoded data in the specified format
 */
function applyDeltaEncoding(data) {
    if (!data || !Array.isArray(data) || data.length === 0) {
        return { 
            startEpoch: 0, 
            startVolume: 0, 
            timeMultiplier: TIME_MULTIPLIER,
            epochs: [], 
            volumes: [] 
        };
    }
    
    // First element becomes the base reference
    const startEpoch = data[0].epoch;
    const startVolume = data[0].volume;
    
    // For each subsequent element, store deltas from previous
    const epochs = [];
    const volumes = [];
    
    for (let i = 1; i < data.length; i++) {
        // Store epoch deltas normalized to reduce zeros
        epochs.push((data[i].epoch - data[i-1].epoch) / TIME_MULTIPLIER);
        volumes.push(data[i].volume - data[i-1].volume);
    }
    
    return {
        startEpoch: startEpoch / TIME_MULTIPLIER,
        startVolume,
        timeMultiplier: TIME_MULTIPLIER,
        epochs,
        volumes
    };
}

/**
 * Decodes delta-encoded data back to original time series
 * 
 * @param {DeltaEncodedData} encodedData - The delta-encoded data
 * @returns {Array<TimeSeriesPoint>} The original time series data
 */
function decodeDeltaEncoding(encodedData) {
    const { startEpoch, startVolume, timeMultiplier = TIME_MULTIPLIER, epochs, volumes } = encodedData;
    
    // Initialize the result array with the first point
    const result = [
        {
            epoch: startEpoch * timeMultiplier,
            volume: startVolume,
            createdAt: parseCreateTime(startEpoch * timeMultiplier)
        }
    ];
    
    // For each delta pair, calculate the original values
    for (let i = 0; i < epochs.length; i++) {
        const prevPoint = result[result.length - 1];
        const currentEpoch = prevPoint.epoch / timeMultiplier + epochs[i];
        
        result.push({
            epoch: currentEpoch * timeMultiplier,
            volume: prevPoint.volume + volumes[i],
            createdAt: parseCreateTime(currentEpoch * timeMultiplier)
        });
    }
    
    return result;
}

module.exports = {
    applyDeltaEncoding,
    decodeDeltaEncoding,
    TIME_MULTIPLIER
}