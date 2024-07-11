<%--
    Licensed to the Apache Software Foundation (ASF) under one or more
    contributor license agreements.  See the NOTICE file distributed with
    this work for additional information regarding copyright ownership.
    The ASF licenses this file to You under the Apache License, Version 2.0
    (the "License"); you may not use this file except in compliance with
    the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0
   
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
--%>
// This script is from https://github.com/shadowmoose/GHA-LoC-Badge/blob/master/src/index.js

const { badgen } = require('badgen');
const fs = require('fs').promises;
const path = require('path');
const core = require('@actions/core');
const { glob } = require('glob-gitignore');


const st = Date.now();
const dir = core.getInput('directory') || './';
const debug = core.getInput('debug') === 'true';
const badge = core.getInput('badge') || './badge.svg';
const patterns = (core.getInput('patterns')||'').split('|').map(s => s.trim()).filter(s=>s);
const ignore = (core.getInput('ignore') || '').split('|').map(s => s.trim()).filter(s=>s);

const badgeOpts = {};
for (const en of Object.keys(process.env)) {
	if (en.startsWith('INPUT_BADGE_')) {
		badgeOpts[en.replace('INPUT_BADGE_', '').toLowerCase()] = process.env[en]
	}
}

if (debug) core.info('Debugging enabled.');


async function countLines(fullPath) {
	return new Promise((res, rej) => {
		let count = 1;
		require('fs').createReadStream(fullPath)
			.on('data', function(chunk) {
				let index = -1;
				while((index = chunk.indexOf(10, index + 1)) > -1) count++
			})
			.on('end', function() {
				res(count);
			})
			.on('error', function(err) {
				rej(err)
			});
	})
}

const countThrottled = throttle(countLines, 10);

/**
 * Recursively count the lines in all matching files within the given directory.
 *
 * @param dir {string} The path to check.
 * @param patterns {string[]} array of patterns to match against.
 * @param negative {string[]} array of patterns to NOT match against.
 * @return {Promise<{ignored: number, lines: number, counted: number}>} An array of all files located, as absolute paths.
 */
async function getFiles (dir, patterns = [], negative = []) {
	let lines = 0, ignored=0, counted=0;

	await glob(patterns, {
		cwd: dir,
		ignore: negative,
		nodir: true
	}).then(files => {
		counted = files.length;
		return Promise.all(files.map( async f => {
			try {
				if (debug) core.info(`Counting: ${f}`);
				return await countThrottled(f);
			} catch (err) {
				core.error(err);
				return 0;
			}
		}))
	}).then(res => res.map(r => lines += r));

	return { lines, ignored, counted };
}

function throttle(callback, limit=5) {
	let idx = 0;
	const queue = new Array(limit);

	return async (...args) => {
		const offset = idx++ % limit;
		const blocker = queue[offset];
		let cb = null;
		queue[offset] = new Promise((res) => cb = res);  // Next call waits for this call's resolution.

		if (blocker) await blocker;
		try {
			return await callback.apply(this, args);
		} finally {
			cb();
		}
	}
}


function makeBadge(text, config) {
	return badgen({
    label: "lines of code",
		status: `${text}`,               // <Text>, required
	});
}


getFiles(dir, patterns, ignore).then( async ret => {
	await fs.mkdir(path.dirname(badge), { recursive: true })
	await fs.writeFile(badge, makeBadge(ret.lines.toLocaleString(), badgeOpts));
})
