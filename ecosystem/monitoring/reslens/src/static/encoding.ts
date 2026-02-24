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


// Define a constant for time normalization
const TIME_MULTIPLIER = 1000000;

/**
 * Decodes delta-encoded data back to original time series
 * 
 * @param {DeltaEncodedData} encodedData - The delta-encoded data
 * @returns {Array<TimeSeriesPoint>} The original time series data
 */
export function decodeDeltaEncoding(encodedData) {
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

/**
 * Converts a Unix timestamp in microseconds to a formatted date string
 * @param {number} createtime - Unix timestamp in microseconds
 * @returns {string} Formatted date string in the format "DD MMM YYYY HH:MM:SS GMT"
 */
export function parseCreateTime(createtime) {
    // Convert microseconds to milliseconds for JavaScript Date
    const milliseconds = Math.floor(createtime / 1000);

    // Create Date object from timestamp
    const date = new Date(milliseconds);

    // Get date components
    const day = date.getUTCDate();
    const month = date.getUTCMonth();
    const year = date.getUTCFullYear();
    const hours = date.getUTCHours();
    const minutes = date.getUTCMinutes();
    const seconds = date.getUTCSeconds();

    // Array of month abbreviations
    const months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

    // Format day with leading zero if needed
    const dayStr = day < 10 ? `0${day}` : `${day}`;

    // Format time components with leading zeros if needed
    const hoursStr = hours < 10 ? `0${hours}` : `${hours}`;
    const minutesStr = minutes < 10 ? `0${minutes}` : `${minutes}`;
    const secondsStr = seconds < 10 ? `0${seconds}` : `${seconds}`;

    // Construct the formatted date string
    return `${dayStr} ${months[month]} ${year} ${hoursStr}:${minutesStr}:${secondsStr} GMT`;
}

export function parseTimeToUnixEpoch(timeString) {
    const months = {
        "Jan": 0, "Feb": 1, "Mar": 2, "Apr": 3, "May": 4, "Jun": 5,
        "Jul": 6, "Aug": 7, "Sep": 8, "Oct": 9, "Nov": 10, "Dec": 11
    };

    // Split the string into parts
    const parts = timeString.split(" ");

    // Extract date components
    const day = parseInt(parts[0], 10);
    const month = months[parts[1]];
    const year = parseInt(parts[2], 10);

    // Extract time components
    const timeParts = parts[3].split(":");
    const hour = parseInt(timeParts[0], 10);
    const minute = parseInt(timeParts[1], 10);
    const second = parseInt(timeParts[2], 10);

    // Create a new Date object in UTC/GMT
    const date = new Date(Date.UTC(year, month, day, hour, minute, second));

    // Get the timestamp in milliseconds and convert to microseconds
    const microseconds = date.getTime() * 1000;

    return microseconds;
}
