/*
 * Utility to add Apache 2.0 license headers to source files.
 * - Skips files/paths per licenserc.yaml and common build dirs
 * - Uses language-appropriate comment syntax
 * - Avoids duplicating header if already present
 */

const fs = require('fs');
const path = require('path');

const repoRoot = process.cwd();

const apacheHeaderLines = [
  'Licensed to the Apache Software Foundation (ASF) under one',
  'or more contributor license agreements.  See the NOTICE file',
  'distributed with this work for additional information',
  'regarding copyright ownership.  The ASF licenses this file',
  'to you under the Apache License, Version 2.0 (the',
  '"License"); you may not use this file except in compliance',
  'with the License.  You may obtain a copy of the License at',
  '',
  '  http://www.apache.org/licenses/LICENSE-2.0',
  '',
  'Unless required by applicable law or agreed to in writing,',
  'software distributed under the License is distributed on an',
  '"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY',
  'KIND, either express or implied.  See the License for the',
  'specific language governing permissions and limitations',
  'under the License.',
];

const ignoreExact = new Set([
  'CNAME',
  'DISCLAIMER',
  'NOTICE',
  'LICENSE',
  'documents/doxygen/.gitignore',
  'third_party/loc_script/src/index.js',
]);

const ignoreExtensions = new Set([
  '.conf',
  '.config',
  '.json',
  '.sol',
  '.pri',
  '.pub',
]);

const ignoreDirs = new Set([
  'node_modules',
  '.git',
  '.next',
  'storybook-static',
  'dist',
  'build',
  '.vercel',
  '.vscode',
  '.idea',
  'public/_pagefind',
]);

/** Determine comment style by extension */
function makeHeaderFor(file) {
  const ext = path.extname(file).toLowerCase();
  const headerText = apacheHeaderLines.join('\n');

  // Languages using /* */
  const blockExts = new Set(['.ts', '.tsx', '.js', '.jsx', '.mjs', '.cjs', '.css']);
  if (blockExts.has(ext)) {
    return `/*\n* ${apacheHeaderLines.join('\n* ')}\n*/\n\n`;
  }

  // Markdown/MDX/HTML
  if (ext === '.md' || ext === '.mdx' || ext === '.html' || ext === '.htm') {
    return `<!--\n${headerText}\n-->\n\n`;
  }

  // YAML/YML/TOML/INI/SH
  const lineCommentExts = new Set(['.yml', '.yaml', '.toml', '.ini', '.env', '.sh']);
  if (lineCommentExts.has(ext)) {
    return `# ${apacheHeaderLines.join('\n# ')}\n\n`;
  }

  // Unknown: default to line comments with '#'
  return `# ${apacheHeaderLines.join('\n# ')}\n\n`;
}

function shouldSkip(file, rel) {
  // Skip dotfiles or anything inside a dot dir
  if (rel.split(path.sep).some((seg) => seg.startsWith('.'))) return true;

  // Skip ignored dirs
  if (rel.split(path.sep).some((seg) => ignoreDirs.has(seg))) return true;

  // Skip exact matches
  if (ignoreExact.has(rel)) return true;
  if (ignoreExact.has(path.basename(rel))) return true;

  // Skip by extension
  const ext = path.extname(rel).toLowerCase();
  if (ignoreExtensions.has(ext)) return true;

  return false;
}

function hasHeader(content) {
  const head = content.slice(0, 1000);
  return head.includes('Licensed to the Apache Software Foundation (ASF)');
}

function processFile(absPath, relPath) {
  const content = fs.readFileSync(absPath, 'utf8');
  if (hasHeader(content)) return false;

  const header = makeHeaderFor(relPath);
  const updated = header + content;
  fs.writeFileSync(absPath, updated, 'utf8');
  return true;
}

function walk(dir, base = '') {
  const entries = fs.readdirSync(dir, { withFileTypes: true });
  for (const e of entries) {
    const rel = path.join(base, e.name);
    const abs = path.join(dir, e.name);
    if (e.isDirectory()) {
      if (ignoreDirs.has(e.name) || e.name.startsWith('.')) continue;
      walk(abs, rel);
    } else if (e.isFile()) {
      if (shouldSkip(abs, rel)) continue;
      try {
        processFile(abs, rel);
      } catch (err) {
        // Ignore binary or unreadable files gracefully
      }
    }
  }
}

walk(repoRoot);
console.log('License headers added where missing.');

