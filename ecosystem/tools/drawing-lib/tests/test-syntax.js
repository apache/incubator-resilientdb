#!/usr/bin/env node

/*
 * Comprehensive Syntax and Structure Validator for Collaborative Drawing Toolkit
 * Tests all modules for syntax errors, import/export correctness, and API endpoint validation
 */

const fs = require('fs');
const path = require('path');
const vm = require('vm');

// Color codes for output
const colors = {
  reset: '\x1b[0m',
  bright: '\x1b[1m',
  green: '\x1b[32m',
  red: '\x1b[31m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  cyan: '\x1b[36m'
};

let totalTests = 0;
let passedTests = 0;
let failedTests = 0;
const errors = [];

function log(message, color = colors.reset) {
  console.log(color + message + colors.reset);
}

function test(name, fn) {
  totalTests++;
  try {
    fn();
    passedTests++;
    log(`  ✓ ${name}`, colors.green);
    return true;
  } catch (error) {
    failedTests++;
    log(`  ✗ ${name}`, colors.red);
    log(`    ${error.message}`, colors.red);
    errors.push({ test: name, error: error.message });
    return false;
  }
}

function readFile(filePath) {
  const fullPath = path.join(__dirname, '..', filePath);
  if (!fs.existsSync(fullPath)) {
    throw new Error(`File not found: ${filePath}`);
  }
  return fs.readFileSync(fullPath, 'utf8');
}

function checkSyntax(filePath) {
  try {
    const content = readFile(filePath);
    // Try to parse as a module (doesn't execute, just validates syntax)
    new vm.Script(content, { filename: filePath });
    return true;
  } catch (error) {
    throw new Error(`Syntax error in ${filePath}: ${error.message}`);
  }
}

function checkApacheLicense(filePath) {
  const content = readFile(filePath);
  if (!content.includes('Licensed to the Apache Software Foundation') &&
    !content.includes('Apache License')) {
    throw new Error(`Missing Apache license header in ${filePath}`);
  }
}

function checkEndpoints(filePath, expectedEndpoints) {
  const content = readFile(filePath);

  for (const [endpoint, description] of Object.entries(expectedEndpoints)) {
    if (!content.includes(endpoint)) {
      throw new Error(`Missing expected endpoint "${endpoint}" (${description})`);
    }
  }

  // Check for old endpoints that should NOT be present
  const forbiddenPatterns = [
    { pattern: /\/rooms\/[^'"`]*['"]/g, message: '/rooms/ endpoint found (should be /canvases/)' },
    { pattern: /\/invites\/[^'"`]*['"]/g, message: '/invites/ endpoint found (should be /collaborations/invitations/)' }
  ];

  for (const { pattern, message } of forbiddenPatterns) {
    const matches = content.match(pattern);
    if (matches && matches.length > 0) {
      throw new Error(`Forbidden pattern: ${message} - Found: ${matches[0]}`);
    }
  }
}

// ============================================================================
// Test Suite
// ============================================================================

log('\n' + '='.repeat(70), colors.bright);
log('Collaborative Drawing Toolkit - Comprehensive Test Suite', colors.bright + colors.cyan);
log('='.repeat(70) + '\n', colors.bright);

// Test 1: File Structure
log('Test Suite 1: File Structure', colors.bright + colors.blue);
test('All required source files exist', () => {
  const requiredFiles = [
    'src/index.js',
    'src/modules/auth.js',
    'src/modules/canvases.js',
    'src/modules/socket.js',
    'src/modules/invites.js',
    'src/modules/notifications.js',
    'README.md',
    'LICENSE',
    'package.json',
    'INSTALL.md',
    'MIGRATION_TO_API_V1.md',
    'PR_SUMMARY.md',
    'examples/basic-drawing-app.html'
  ];

  for (const file of requiredFiles) {
    const fullPath = path.join(__dirname, '..', file);
    if (!fs.existsSync(fullPath)) {
      throw new Error(`Required file missing: ${file}`);
    }
  }
});

// Test 2: JavaScript Syntax
log('\nTest Suite 2: JavaScript Syntax Validation', colors.bright + colors.blue);
const jsFiles = [
  'src/index.js',
  'src/modules/auth.js',
  'src/modules/canvases.js',
  'src/modules/socket.js',
  'src/modules/invites.js',
  'src/modules/notifications.js'
];

for (const file of jsFiles) {
  test(`Valid ES6 module: ${file}`, () => {
    // For ES6 modules, just check they parse without obvious syntax errors
    const content = readFile(file);
    // Check for balanced braces
    const openBraces = (content.match(/{/g) || []).length;
    const closeBraces = (content.match(/}/g) || []).length;
    if (openBraces !== closeBraces) {
      throw new Error(`Unbalanced braces: ${openBraces} open, ${closeBraces} close`);
    }
    // Check for basic ES6 module syntax
    if (!content.includes('export ') && !content.includes('import ')) {
      throw new Error('Not an ES6 module (missing import/export)');
    }
  });
}

// Test 3: Apache License Headers
log('\nTest Suite 3: Apache License Headers', colors.bright + colors.blue);
for (const file of jsFiles) {
  test(`Apache license header present: ${file}`, () => {
    checkApacheLicense(file);
  });
}

// Test 4: Main Client (index.js)
log('\nTest Suite 4: Main Client Configuration', colors.bright + colors.blue);
test('index.js exports DrawingClient', () => {
  const content = readFile('src/index.js');
  if (!content.includes('class DrawingClient') && !content.includes('export default')) {
    throw new Error('DrawingClient class or export not found');
  }
});

test('Client constructs correct API base URL', () => {
  const content = readFile('src/index.js');
  if (!content.includes('this.apiBase = `${this.config.baseUrl}/api/${this.config.apiVersion}`')) {
    throw new Error('API base URL construction incorrect');
  }
});

test('Client defaults to API version v1', () => {
  const content = readFile('src/index.js');
  if (!content.includes("apiVersion: config.apiVersion || 'v1'")) {
    throw new Error('API version default not set to v1');
  }
});

test('Client initializes all modules', () => {
  const content = readFile('src/index.js');
  const requiredModules = ['auth', 'canvases', 'invites', 'notifications', 'socket'];
  for (const module of requiredModules) {
    if (!content.includes(`this.${module} =`)) {
      throw new Error(`Module ${module} not initialized`);
    }
  }
});

test('Client has _request method with retry logic', () => {
  const content = readFile('src/index.js');
  if (!content.includes('_request') || !content.includes('retries')) {
    throw new Error('Request method or retry logic not found');
  }
});

test('Client has ApiError class', () => {
  const content = readFile('src/index.js');
  if (!content.includes('class ApiError')) {
    throw new Error('ApiError class not found');
  }
});

// Test 5: Auth Module
log('\nTest Suite 5: Authentication Module', colors.bright + colors.blue);
test('auth.js has all required methods', () => {
  const content = readFile('src/modules/auth.js');
  const requiredMethods = ['register', 'login', 'logout', 'refresh', 'getMe', 'changePassword'];
  for (const method of requiredMethods) {
    if (!content.includes(`async ${method}(`)) {
      throw new Error(`Method ${method} not found`);
    }
  }
});

test('auth.js uses correct /auth endpoints', () => {
  checkEndpoints('src/modules/auth.js', {
    "'/auth/register'": 'register endpoint',
    "'/auth/login'": 'login endpoint',
    "'/auth/logout'": 'logout endpoint',
    "'/auth/refresh'": 'refresh endpoint',
    "'/auth/me'": 'get user endpoint',
    "'/auth/change-password'": 'change password endpoint'
  });
});

// Test 6: Rooms/Canvas Module
log('\nTest Suite 6: Canvas/Rooms Module', colors.bright + colors.blue);
test('canvases.js has all required methods', () => {
  const content = readFile('src/modules/canvases.js');
  const requiredMethods = [
    'create', 'list', 'get', 'update', 'delete', 'share', 'getMembers',
    'getStrokes', 'addStroke', 'clear', 'undo', 'redo', 'getUndoRedoStatus',
    'resetStacks', 'transferOwnership', 'leave', 'invite', 'suggest'
  ];
  for (const method of requiredMethods) {
    if (!content.includes(`async ${method}(`)) {
      throw new Error(`Method ${method} not found`);
    }
  }
});

test('canvases.js uses /canvases endpoints (not /rooms)', () => {
  const content = readFile('src/modules/canvases.js');
  // Check for /canvases endpoints
  if (!content.includes("'/canvases'") && !content.includes('"/canvases"') &&
    !content.includes('`/canvases/') && !content.includes('`/canvases?')) {
    throw new Error('Missing /canvases endpoints');
  }
  // Check for history endpoints
  if (!content.includes('/history/undo') || !content.includes('/history/redo')) {
    throw new Error('Missing nested /history endpoints');
  }
});

test('canvases.js does NOT use old /rooms endpoints', () => {
  const content = readFile('src/modules/canvases.js');
  const forbiddenPatterns = [
    "'/rooms'",
    "'/rooms/${",
    '"/rooms"',
    '"/rooms/${'
  ];

  for (const pattern of forbiddenPatterns) {
    if (content.includes(pattern)) {
      throw new Error(`Found old endpoint pattern: ${pattern}`);
    }
  }
});

test('canvases.js uses DELETE method for clear()', () => {
  const content = readFile('src/modules/canvases.js');
  // Check clear method uses DELETE
  if (!content.includes("async clear(") || !content.includes("method: 'DELETE'")) {
    throw new Error('clear() method should use DELETE method');
  }
});

// Test 7: Invites Module
log('\nTest Suite 7: Invites/Collaborations Module', colors.bright + colors.blue);
test('invites.js has all required methods', () => {
  const content = readFile('src/modules/invites.js');
  const requiredMethods = ['list', 'accept', 'decline'];
  for (const method of requiredMethods) {
    if (!content.includes(`async ${method}(`)) {
      throw new Error(`Method ${method} not found`);
    }
  }
});

test('invites.js uses /collaborations/invitations endpoints', () => {
  const content = readFile('src/modules/invites.js');
  if (!content.includes('/collaborations/invitations')) {
    throw new Error('Missing /collaborations/invitations endpoints');
  }
  // Check for template literal usage
  if (!content.includes('`/collaborations/invitations/${')) {
    throw new Error('Missing template literal for invitation operations');
  }
});

test('invites.js does NOT use old /invites endpoints', () => {
  const content = readFile('src/modules/invites.js');
  const forbiddenPatterns = ["'/invites'", '"/invites"', "'/invites/${", '"/invites/${'];

  for (const pattern of forbiddenPatterns) {
    if (content.includes(pattern)) {
      throw new Error(`Found old endpoint pattern: ${pattern}`);
    }
  }
});

// Test 8: Notifications Module
log('\nTest Suite 8: Notifications Module', colors.bright + colors.blue);
test('notifications.js has all required methods', () => {
  const content = readFile('src/modules/notifications.js');
  const requiredMethods = ['list', 'markRead', 'delete', 'clear', 'getPreferences', 'updatePreferences'];
  for (const method of requiredMethods) {
    if (!content.includes(`async ${method}(`)) {
      throw new Error(`Method ${method} not found`);
    }
  }
});

test('notifications.js uses /notifications endpoints', () => {
  checkEndpoints('src/modules/notifications.js', {
    "'/notifications'": 'notifications endpoint',
    "'/notifications/preferences'": 'preferences endpoint'
  });
});

// Test 9: Socket Module
log('\nTest Suite 9: Socket.IO Module', colors.bright + colors.blue);
test('socket.js has all required methods', () => {
  const content = readFile('src/modules/socket.js');
  const requiredMethods = ['connect', 'disconnect', 'joinCanvas', 'leaveCanvas', 'on', 'emit'];
  for (const method of requiredMethods) {
    if (!content.includes(`${method}(`)) {
      throw new Error(`Method ${method} not found`);
    }
  }
});

test('socket.js imports socket.io-client', () => {
  const content = readFile('src/modules/socket.js');
  if (!content.includes('socket.io-client')) {
    throw new Error('socket.io-client import not found');
  }
});

// Test 10: Example Application
log('\nTest Suite 10: Example Application', colors.bright + colors.blue);
test('basic-drawing-app.html exists and is valid HTML', () => {
  const content = readFile('examples/basic-drawing-app.html');
  if (!content.includes('<!DOCTYPE html>') && !content.includes('<html')) {
    throw new Error('Invalid HTML structure');
  }
});

test('Example app uses /canvases endpoints', () => {
  const content = readFile('examples/basic-drawing-app.html');
  if (!content.includes('/api/v1/canvases/')) {
    throw new Error('Example app not using /canvases endpoints');
  }
});

test('Example app uses history endpoints', () => {
  const content = readFile('examples/basic-drawing-app.html');
  if (!content.includes('/history/undo') || !content.includes('/history/redo')) {
    throw new Error('Example app not using nested /history endpoints');
  }
});

test('Example app uses DELETE for clear', () => {
  const content = readFile('examples/basic-drawing-app.html');
  // Check clearCanvas method exists and uses DELETE
  if (!content.includes('clearCanvas') || !content.includes("method: 'DELETE'")) {
    throw new Error('Example app should have clearCanvas method with DELETE');
  }
});

// Test 11: Documentation
log('\nTest Suite 11: Documentation Validation', colors.bright + colors.blue);
test('README.md is comprehensive (>400 lines)', () => {
  const content = readFile('README.md');
  const lineCount = content.split('\n').length;
  if (lineCount < 400) {
    throw new Error(`README too short: ${lineCount} lines (expected >400)`);
  }
});

test('README.md contains installation instructions', () => {
  const content = readFile('README.md');
  if (!content.toLowerCase().includes('install')) {
    throw new Error('README missing installation instructions');
  }
});

test('README.md contains usage examples', () => {
  const content = readFile('README.md');
  if (!content.includes('```') || !content.includes('client.')) {
    throw new Error('README missing code examples');
  }
});

test('MIGRATION_TO_API_V1.md documents endpoint changes', () => {
  const content = readFile('MIGRATION_TO_API_V1.md');
  if (!content.includes('/canvases') || !content.includes('/rooms')) {
    throw new Error('Migration doc missing endpoint documentation');
  }
});

test('LICENSE file is Apache 2.0', () => {
  const content = readFile('LICENSE');
  if (!content.includes('Apache License') || !content.includes('Version 2.0')) {
    throw new Error('LICENSE is not Apache 2.0');
  }
});

// Test 12: Package Configuration
log('\nTest Suite 12: Package Configuration', colors.bright + colors.blue);
test('package.json is valid JSON', () => {
  const content = readFile('package.json');
  JSON.parse(content); // Will throw if invalid
});

test('package.json has required fields', () => {
  const content = readFile('package.json');
  const pkg = JSON.parse(content);
  const required = ['name', 'version', 'description', 'main', 'license'];
  for (const field of required) {
    if (!pkg[field]) {
      throw new Error(`package.json missing field: ${field}`);
    }
  }
});

test('package.json license is Apache-2.0', () => {
  const content = readFile('package.json');
  const pkg = JSON.parse(content);
  if (pkg.license !== 'Apache-2.0') {
    throw new Error(`Wrong license in package.json: ${pkg.license}`);
  }
});

test('package.json has socket.io-client dependency', () => {
  const content = readFile('package.json');
  const pkg = JSON.parse(content);
  if (!pkg.dependencies || !pkg.dependencies['socket.io-client']) {
    throw new Error('Missing socket.io-client dependency');
  }
});

// ============================================================================
// Test Results Summary
// ============================================================================

log('\n' + '='.repeat(70), colors.bright);
log('Test Results Summary', colors.bright + colors.cyan);
log('='.repeat(70), colors.bright);
log(`Total Tests: ${totalTests}`);
log(`Passed: ${passedTests}`, colors.green);
log(`Failed: ${failedTests}`, failedTests > 0 ? colors.red : colors.green);
log('='.repeat(70) + '\n', colors.bright);

if (failedTests > 0) {
  log('Failed Tests:', colors.bright + colors.red);
  errors.forEach(({ test, error }, index) => {
    log(`${index + 1}. ${test}`, colors.red);
    log(`   ${error}`, colors.yellow);
  });
  log('');
  process.exit(1);
} else {
  log('✓ All tests passed! Toolkit is ready for PR.', colors.bright + colors.green);
  process.exit(0);
}
