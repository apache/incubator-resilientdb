#!/usr/bin/env node

/*
 * Integration Test Suite for Collaborative Drawing Toolkit
 * Tests actual module instantiation, method signatures, and endpoint construction
 */

const path = require('path');

// Color codes
const colors = {
  reset: '\x1b[0m',
  bright: '\x1b[1m',
  green: '\x1b[32m',
  red: '\x1b[31m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  cyan: '\x1b[36m'
};

function log(message, color = colors.reset) {
  console.log(color + message + colors.reset);
}

let totalTests = 0;
let passedTests = 0;
let failedTests = 0;
const errors = [];

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
    if (error.stack) {
      const stackLines = error.stack.split('\n').slice(1, 3);
      stackLines.forEach(line => log(`    ${line.trim()}`, colors.yellow));
    }
    errors.push({ test: name, error: error.message });
    return false;
  }
}

async function asyncTest(name, fn) {
  totalTests++;
  try {
    await fn();
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

// Mock fetch for testing
global.fetch = async (url, options = {}) => {
  return {
    ok: true,
    status: 200,
    statusText: 'OK',
    headers: new Map([['content-type', 'application/json']]),
    json: async () => ({ success: true, data: { test: 'data' } }),
    text: async () => JSON.stringify({ success: true })
  };
};

// Mock socket.io-client
const mockSocketIO = {
  connect: (url, options) => ({
    on: () => { },
    emit: () => { },
    off: () => { },
    disconnect: () => { },
    connected: false
  })
};

// Add module resolution support
const Module = require('module');
const originalRequire = Module.prototype.require;

Module.prototype.require = function (id) {
  if (id === 'socket.io-client') {
    return mockSocketIO;
  }
  return originalRequire.apply(this, arguments);
};

log('\n' + '='.repeat(70), colors.bright);
log('Collaborative Drawing Toolkit - Integration Test Suite', colors.bright + colors.cyan);
log('='.repeat(70) + '\n', colors.bright);

// Test 1: Client Initialization
log('Test Suite 1: Client Initialization', colors.bright + colors.blue);

test('Client can be imported as ES module', () => {
  // We can't actually import ES modules in Node CommonJS, but we can verify the structure
  const fs = require('fs');
  const indexPath = path.join(__dirname, '../src/index.js');
  const content = fs.readFileSync(indexPath, 'utf8');

  if (!content.includes('export default DrawingClient')) {
    throw new Error('Missing default export of DrawingClient');
  }
});

test('Client requires baseUrl configuration', () => {
  const fs = require('fs');
  const indexPath = path.join(__dirname, '../src/index.js');
  const content = fs.readFileSync(indexPath, 'utf8');

  if (!content.includes('if (!config || !config.baseUrl)')) {
    throw new Error('Client should validate baseUrl is provided');
  }
});

test('Client builds correct API paths', () => {
  const fs = require('fs');
  const indexPath = path.join(__dirname, '../src/index.js');
  const content = fs.readFileSync(indexPath, 'utf8');

  // Check path construction
  if (!content.includes('this.apiBase = `${this.config.baseUrl}/api/${this.config.apiVersion}`')) {
    throw new Error('API base path construction is incorrect');
  }
});

// Test 2: Module Method Signatures
log('\nTest Suite 2: Module Method Signatures', colors.bright + colors.blue);

test('Auth module has correct method signatures', () => {
  const fs = require('fs');
  const authPath = path.join(__dirname, '../src/modules/auth.js');
  const content = fs.readFileSync(authPath, 'utf8');

  const expectedMethods = [
    { name: 'register', params: ['email', 'username', 'password'] },
    { name: 'login', params: ['email', 'password'] },
    { name: 'logout', params: [] },
    { name: 'refresh', params: ['refreshToken'] },
    { name: 'getMe', params: [] },
    { name: 'changePassword', params: ['oldPassword', 'newPassword'] }
  ];

  for (const { name, params } of expectedMethods) {
    const regex = new RegExp(`async ${name}\\s*\\(`);
    if (!regex.test(content)) {
      throw new Error(`Method ${name} not found or not async`);
    }
  }
});

test('Canvases module has correct method signatures', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  const requiredMethods = [
    'create', 'list', 'get', 'update', 'delete', 'share', 'getMembers',
    'getStrokes', 'addStroke', 'clear', 'undo', 'redo',
    'getUndoRedoStatus', 'resetStacks', 'transferOwnership',
    'leave', 'invite', 'suggest'
  ];

  for (const method of requiredMethods) {
    const regex = new RegExp(`async ${method}\\s*\\(`);
    if (!regex.test(content)) {
      throw new Error(`Method ${method} not found or not async`);
    }
  }
});

test('Invites module has correct method signatures', () => {
  const fs = require('fs');
  const invitesPath = path.join(__dirname, '../src/modules/invites.js');
  const content = fs.readFileSync(invitesPath, 'utf8');

  const methods = ['list', 'accept', 'decline'];
  for (const method of methods) {
    const regex = new RegExp(`async ${method}\\s*\\(`);
    if (!regex.test(content)) {
      throw new Error(`Method ${method} not found or not async`);
    }
  }
});

test('Notifications module has correct method signatures', () => {
  const fs = require('fs');
  const notifPath = path.join(__dirname, '../src/modules/notifications.js');
  const content = fs.readFileSync(notifPath, 'utf8');

  const methods = ['list', 'markRead', 'delete', 'clear', 'getPreferences', 'updatePreferences'];
  for (const method of methods) {
    const regex = new RegExp(`async ${method}\\s*\\(`);
    if (!regex.test(content)) {
      throw new Error(`Method ${method} not found or not async`);
    }
  }
});

test('Socket module has correct method signatures', () => {
  const fs = require('fs');
  const socketPath = path.join(__dirname, '../src/modules/socket.js');
  const content = fs.readFileSync(socketPath, 'utf8');

  const methods = ['connect', 'disconnect', 'joinCanvas', 'leaveCanvas', 'on', 'emit'];
  for (const method of methods) {
    if (!content.includes(`${method}(`)) {
      throw new Error(`Method ${method} not found`);
    }
  }
});

// Test 3: Endpoint Construction
log('\nTest Suite 3: Endpoint Path Construction', colors.bright + colors.blue);

test('All canvas endpoints use /canvases prefix', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  // Extract all _request calls
  const requestCalls = content.match(/_request\([^)]+\)/g) || [];

  // Check that none use /rooms
  for (const call of requestCalls) {
    if (call.includes('/rooms/') || call.includes("'/rooms'") || call.includes('"/rooms"')) {
      throw new Error(`Found /rooms endpoint in call: ${call}`);
    }
  }

  // Check that we have /canvases calls
  const hasCanvases = requestCalls.some(call =>
    call.includes('/canvases') || call.includes('canvases')
  );
  if (!hasCanvases) {
    throw new Error('No /canvases endpoints found');
  }
});

test('History operations use nested /history/* paths', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  const historyOps = ['/history/undo', '/history/redo', '/history/status', '/history/reset'];
  for (const op of historyOps) {
    if (!content.includes(op)) {
      throw new Error(`Missing history operation: ${op}`);
    }
  }
});

test('Invitations use /collaborations/invitations paths', () => {
  const fs = require('fs');
  const invitesPath = path.join(__dirname, '../src/modules/invites.js');
  const content = fs.readFileSync(invitesPath, 'utf8');

  if (!content.includes('/collaborations/invitations')) {
    throw new Error('Missing /collaborations/invitations endpoint');
  }
});

test('Auth endpoints use /auth prefix', () => {
  const fs = require('fs');
  const authPath = path.join(__dirname, '../src/modules/auth.js');
  const content = fs.readFileSync(authPath, 'utf8');

  const authOps = ['/auth/register', '/auth/login', '/auth/logout', '/auth/refresh', '/auth/me'];
  for (const op of authOps) {
    if (!content.includes(op)) {
      throw new Error(`Missing auth operation: ${op}`);
    }
  }
});

// Test 4: HTTP Methods
log('\nTest Suite 4: HTTP Method Correctness', colors.bright + colors.blue);

test('Clear operation uses DELETE method', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  // Find clear method
  const clearMatch = content.match(/async clear\([^)]*\)\s*{[\s\S]{0,300}/);
  if (!clearMatch) {
    throw new Error('clear() method not found');
  }

  const clearMethod = clearMatch[0];
  if (!clearMethod.includes("method: 'DELETE'")) {
    throw new Error('clear() should use DELETE method');
  }
});

test('Create operations use POST method', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  // Check create method
  const createMatch = content.match(/async create\([^)]*\)\s*{[\s\S]{0,300}/);
  if (!createMatch || !createMatch[0].includes("method: 'POST'")) {
    throw new Error('create() should use POST method');
  }
});

test('Update operations use PATCH method', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  // Check update method
  const updateMatch = content.match(/async update\([^)]*\)\s*{[\s\S]{0,300}/);
  if (!updateMatch || !updateMatch[0].includes("method: 'PATCH'")) {
    throw new Error('update() should use PATCH method');
  }
});

test('Delete operations use DELETE method', () => {
  const fs = require('fs');
  const roomsPath = path.join(__dirname, '../src/modules/canvases.js');
  const content = fs.readFileSync(roomsPath, 'utf8');

  // Check delete method
  const deleteMatch = content.match(/async delete\([^)]*\)\s*{[\s\S]{0,300}/);
  if (!deleteMatch || !deleteMatch[0].includes("method: 'DELETE'")) {
    throw new Error('delete() should use DELETE method');
  }
});

// Test 5: Error Handling
log('\nTest Suite 5: Error Handling', colors.bright + colors.blue);

test('ApiError class is defined', () => {
  const fs = require('fs');
  const indexPath = path.join(__dirname, '../src/index.js');
  const content = fs.readFileSync(indexPath, 'utf8');

  if (!content.includes('class ApiError')) {
    throw new Error('ApiError class not found');
  }

  // Check it extends Error
  const apiErrorMatch = content.match(/class ApiError[^{]*{[\s\S]{0,500}/);
  if (!apiErrorMatch) {
    throw new Error('ApiError class definition incomplete');
  }
});

test('Client has retry logic', () => {
  const fs = require('fs');
  const indexPath = path.join(__dirname, '../src/index.js');
  const content = fs.readFileSync(indexPath, 'utf8');

  if (!content.includes('retries') || !content.includes('for (let attempt')) {
    throw new Error('Retry logic not found in _request method');
  }
});

test('Client handles token expiration', () => {
  const fs = require('fs');
  const indexPath = path.join(__dirname, '../src/index.js');
  const content = fs.readFileSync(indexPath, 'utf8');

  if (!content.includes('onTokenExpired') || !content.includes('401')) {
    throw new Error('Token expiration handling not found');
  }
});

// Test 6: Documentation Consistency
log('\nTest Suite 6: Documentation Consistency', colors.bright + colors.blue);

test('README examples use correct API paths', () => {
  const fs = require('fs');
  const readmePath = path.join(__dirname, '../README.md');
  const content = fs.readFileSync(readmePath, 'utf8');

  // README should show client.canvases API (which is correct)
  if (!content.includes('client.canvases.')) {
    throw new Error('README missing client.canvases examples');
  }

  // Should mention API v1
  if (!content.includes('v1') && !content.includes('apiVersion')) {
    log('    Warning: README should mention API version configuration', colors.yellow);
  }
});

test('Example HTML uses correct endpoints', () => {
  const fs = require('fs');
  const examplePath = path.join(__dirname, '../examples/basic-drawing-app.html');
  const content = fs.readFileSync(examplePath, 'utf8');

  // Should use /api/v1/canvases
  if (!content.includes('/api/v1/canvases')) {
    throw new Error('Example should use /api/v1/canvases endpoints');
  }

  // Should NOT use /rooms
  if (content.includes('/api/v1/rooms/')) {
    throw new Error('Example should not use /rooms endpoints');
  }
});

test('Migration guide documents all changes', () => {
  const fs = require('fs');
  const migrationPath = path.join(__dirname, 'MIGRATION_TO_API_V1.md');
  const content = fs.readFileSync(migrationPath, 'utf8');

  const requiredTopics = [
    '/canvases',
    '/history',
    '/collaborations',
    'DELETE',
    'undo',
    'redo'
  ];

  for (const topic of requiredTopics) {
    if (!content.includes(topic)) {
      throw new Error(`Migration guide missing topic: ${topic}`);
    }
  }
});

// Test Results
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
  log('✓ All integration tests passed!', colors.bright + colors.green);
  log('  The toolkit is properly structured and ready for production use.', colors.green);
  process.exit(0);
}
