/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

const { exec } = require('child_process');
const util = require('util');

// Promisified version of exec
const execAsync = util.promisify(exec);

/**
 * Convert pprof profile to markdown format
 * @param {string} profilePath - Path to the .pprof file
 * @returns {Promise<string>} - Markdown representation of the profile
 */
async function convertPprofToMarkdown(profilePath) {
  try {
    // Run pprof command to get text output
    const cmd = `go tool pprof -text ${profilePath}`;
    const { stdout, stderr } = await execAsync(cmd);
    
    if (stderr && !stderr.includes('Main binary filename not available')) {
      console.error('pprof warning:', stderr);
    }
    
    // Parse pprof text output
    const data = parsePprofText(stdout);
    
    // Convert to markdown
    return toMarkdown(data);
  } catch (error) {
    console.error(`Error converting profile: ${error.message}`);
    throw new Error(`Failed to convert profile: ${error.message}`);
  }
}

/**
 * Parse pprof text output into structured format
 */
function parsePprofText(content) {
  // Extract header information
  const timeMatch = content.match(/Time: (.*)/);
  const timeStr = timeMatch ? timeMatch[1] : 'Unknown';
  
  const samplesMatch = content.match(/Showing nodes accounting for (\d+)samples, ([\d.]+)% of (\d+)samples total/);
  let shownSamples = 0, shownPercent = 0, totalSamples = 0;
  
  if (samplesMatch) {
    shownSamples = parseInt(samplesMatch[1], 10);
    shownPercent = parseFloat(samplesMatch[2]);
    totalSamples = parseInt(samplesMatch[3], 10);
  }
  
  const droppedMatch = content.match(/Dropped (\d+) nodes \(cum <= (\d+)samples\)/);
  const droppedNodes = droppedMatch ? parseInt(droppedMatch[1], 10) : 0;
  
  // Parse profiling data
  const dataLines = content.split('\n');
  let startIdx = 0;
  
  for (let i = 0; i < dataLines.length; i++) {
    if (dataLines[i].trim().startsWith('flat  flat%')) {
      startIdx = i + 1;
      break;
    }
  }
  
  const profileEntries = [];
  const lineRegex = /(\d+)samples\s+([\d.]+)%\s+([\d.]+)%\s+(\d+)samples\s+([\d.]+)%\s+(.+)/;
  
  for (let i = startIdx; i < dataLines.length; i++) {
    const line = dataLines[i].trim();
    if (!line) continue;
    
    const match = line.match(lineRegex);
    if (match) {
      const entry = {
        flat_samples: parseInt(match[1], 10),
        flat_percent: parseFloat(match[2]),
        sum_percent: parseFloat(match[3]),
        cum_samples: parseInt(match[4], 10),
        cum_percent: parseFloat(match[5]),
        function: match[6].trim()
      };
      profileEntries.push(entry);
    }
  }
  
  // Build the result
  return {
    metadata: {
      time: timeStr,
      total_samples: totalSamples,
      shown_samples: shownSamples,
      shown_percent: shownPercent,
      dropped_nodes: droppedNodes
    },
    profile_entries: profileEntries
  };
}

/**
 * Convert data to Markdown
 */
function toMarkdown(data) {
  let md = [`# CPU Profile Analysis\n\n`];
  md.push(`**Time:** ${data.metadata.time}\n\n`);
  md.push(`**Samples:** ${data.metadata.shown_samples} of ${data.metadata.total_samples} (${data.metadata.shown_percent}%)\n\n`);
  md.push(`**Dropped Nodes:** ${data.metadata.dropped_nodes}\n\n`);
  
  md.push(`## Profile Data\n\n`);
  md.push(`| Flat Samples | Flat % | Sum % | Cum Samples | Cum % | Function |\n`);
  md.push(`|-------------|--------|-------|-------------|-------|----------|\n`);
  
  for (const entry of data.profile_entries) {
    md.push(`| ${entry.flat_samples} | ${entry.flat_percent}% | ${entry.sum_percent}% | ` +
           `${entry.cum_samples} | ${entry.cum_percent}% | \`${entry.function}\` |\n`);
  }
  
  // Add summary section with insights
  md.push(`\n## Performance Insights\n\n`);
  
  // Top functions by flat and cumulative time
  const flatTop = [...data.profile_entries]
    .sort((a, b) => b.flat_samples - a.flat_samples)
    .slice(0, 5);
    
  const cumTop = [...data.profile_entries]
    .sort((a, b) => b.cum_samples - a.cum_samples)
    .slice(0, 5);
  
  md.push(`### Top Functions by Self Time\n\n`);
  flatTop.forEach((entry, i) => {
    md.push(`${i+1}. \`${entry.function}\` - ${entry.flat_percent}% of total time\n`);
  });
  
  md.push(`\n### Top Functions by Cumulative Time\n\n`);
  cumTop.forEach((entry, i) => {
    md.push(`${i+1}. \`${entry.function}\` - ${entry.cum_percent}% of total time\n`);
  });
  
  return md.join('');
}

module.exports = {
  convertPprofToMarkdown
};
