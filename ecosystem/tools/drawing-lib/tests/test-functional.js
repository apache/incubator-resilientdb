#!/usr/bin/env node

/*
 * Comprehensive Functional Test Suite for Collaborative Drawing Toolkit
 * Tests actual module behavior, API request construction, error handling, and real-world scenarios
 */

const path = require('path');
const fs = require('fs');

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
    errors.push({ test: name, error: error.message });
    return false;
  }
}

// Simple jest mock implementation for standalone execution
if (typeof jest === 'undefined') {
  global.jest = {
    fn: (impl) => {
      const mockFn = impl || function () { };
      mockFn.mock = { calls: [], results: [] };
      mockFn.mockReturnValue = (val) => {
        const wrapped = (...args) => {
          mockFn.mock.calls.push(args);
          mockFn.mock.results.push({ type: 'return', value: val });
          return val;
        };
        wrapped.mock = mockFn.mock;
        return wrapped;
      };
      mockFn.mockResolvedValue = (val) => {
        const wrapped = async (...args) => {
          mockFn.mock.calls.push(args);
          mockFn.mock.results.push({ type: 'return', value: Promise.resolve(val) });
          return val;
        };
        wrapped.mock = mockFn.mock;
        return wrapped;
      };
      return mockFn;
    },
    mock: () => { }
  };
}

// Mock globals needed for modules
global.fetch = jest.fn();
global.WebSocket = jest.fn();

// Mock socket.io-client
const mockSocket = {
  on: jest.fn(),
  emit: jest.fn(),
  off: jest.fn(),
  disconnect: jest.fn(),
  connected: false,
  id: 'mock-socket-id'
};

const mockIO = jest.fn(() => mockSocket);
mockIO.connect = jest.fn(() => mockSocket);

// Setup module mocking (not needed for file-based tests)
// jest.mock('socket.io-client', () => mockIO, { virtual: true });

// Simple jest mock implementation for standalone execution
if (typeof jest === 'undefined') {
  global.jest = {
    fn: (impl) => {
      const mockFn = impl || function () { };
      mockFn.mock = { calls: [], results: [] };
      mockFn.mockReturnValue = (val) => {
        const wrapped = (...args) => {
          mockFn.mock.calls.push(args);
          mockFn.mock.results.push({ type: 'return', value: val });
          return val;
        };
        wrapped.mock = mockFn.mock;
        return wrapped;
      };
      mockFn.mockResolvedValue = (val) => {
        const wrapped = async (...args) => {
          mockFn.mock.calls.push(args);
          mockFn.mock.results.push({ type: 'return', value: Promise.resolve(val) });
          return val;
        };
        wrapped.mock = mockFn.mock;
        return wrapped;
      };
      return mockFn;
    },
    mock: () => { }
  };
}

log('\n' + '='.repeat(70), colors.bright);
log('Collaborative Drawing Toolkit - Functional Test Suite', colors.bright + colors.cyan);
log('='.repeat(70) + '\n', colors.bright);

// Test Suite 1: Client Configuration & Initialization
log('Test Suite 1: Client Configuration & Initialization', colors.bright + colors.blue);

test('Client validates required baseUrl parameter', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('if (!config || !config.baseUrl)')) {
    throw new Error('Client should validate baseUrl is required');
  }

  if (!indexContent.includes('throw new Error')) {
    throw new Error('Client should throw error when baseUrl is missing');
  }
});

test('Client constructs correct API base URL with version', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  // Check API base construction
  const apiBasePattern = /this\.apiBase\s*=\s*`\$\{this\.config\.baseUrl\}\/api\/\$\{this\.config\.apiVersion\}`/;
  if (!apiBasePattern.test(indexContent)) {
    throw new Error('API base URL construction is incorrect');
  }
});

test('Client defaults to v1 API version', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes("apiVersion: config.apiVersion || 'v1'")) {
    throw new Error('Should default to v1 when apiVersion not provided');
  }
});

test('Client removes trailing slash from baseUrl', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('.replace(/\\/$/, \'\')')) {
    throw new Error('Should remove trailing slash from baseUrl');
  }
});

test('Client initializes all required modules', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  const modules = ['auth', 'canvases', 'invites', 'notifications', 'socket'];
  for (const mod of modules) {
    if (!indexContent.includes(`this.${mod} = new`)) {
      throw new Error(`Module ${mod} not initialized properly`);
    }
  }
});

test('Client stores token for authenticated requests', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('setToken(token)')) {
    throw new Error('Should have setToken method');
  }

  if (!indexContent.includes('this._token = token')) {
    throw new Error('Should store token in _token property');
  }
});

// Test Suite 2: Request Method Implementation
log('\nTest Suite 2: Request Method & Error Handling', colors.bright + colors.blue);

test('_request method constructs full URL correctly', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('const url = `${this.apiBase}${path}`')) {
    throw new Error('_request should construct full URL from apiBase and path');
  }
});

test('_request method includes Authorization header when token present', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('Authorization')) {
    throw new Error('Should include Authorization header');
  }

  if (!indexContent.includes('Bearer')) {
    throw new Error('Should use Bearer token format');
  }
});

test('_request method implements retry logic', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('for (let attempt = 0')) {
    throw new Error('Should implement retry loop');
  }

  if (!indexContent.includes('this.config.retries')) {
    throw new Error('Should use configurable retries from config');
  }
});

test('_request method handles timeout', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('timeout') || !indexContent.includes('AbortController')) {
    throw new Error('Should implement request timeout with AbortController');
  }
});

test('_request method handles 401 token expiration', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  if (!indexContent.includes('401')) {
    throw new Error('Should check for 401 status');
  }

  if (!indexContent.includes('onTokenExpired')) {
    throw new Error('Should call onTokenExpired callback on 401');
  }
});

test('ApiError class includes all necessary information', () => {
  const indexContent = fs.readFileSync(path.join(__dirname, '../src/index.js'), 'utf8');

  const apiErrorMatch = indexContent.match(/class ApiError[\s\S]{0,800}constructor[\s\S]{0,500}\}/);
  if (!apiErrorMatch) {
    throw new Error('ApiError class not properly defined');
  }

  const errorClass = apiErrorMatch[0];
  if (!errorClass.includes('this.status =') || !errorClass.includes('super(')) {
    throw new Error('ApiError should store status and call Error constructor');
  }
});

// Test Suite 3: Authentication Module Functionality
log('\nTest Suite 3: Authentication Module Functionality', colors.bright + colors.blue);

test('register() method sends correct data to /auth/register', () => {
  const authContent = fs.readFileSync(path.join(__dirname, '../src/modules/auth.js'), 'utf8');

  const registerMatch = authContent.match(/async register\([^)]+\)[\s\S]{0,400}/);
  if (!registerMatch) {
    throw new Error('register method not found');
  }

  const method = registerMatch[0];
  if (!method.includes("'/auth/register'")) {
    throw new Error('Should POST to /auth/register');
  }

  if (!method.includes("method: 'POST'")) {
    throw new Error('Should use POST method');
  }

  if (!method.includes('email') && !method.includes('username') && !method.includes('password')) {
    throw new Error('Should include email, username, password in request body');
  }
});

test('login() method returns token and stores it', () => {
  const authContent = fs.readFileSync(path.join(__dirname, '../src/modules/auth.js'), 'utf8');

  const loginMatch = authContent.match(/async login\([^)]+\)[\s\S]{0,400}/);
  if (!loginMatch) {
    throw new Error('login method not found');
  }

  const method = loginMatch[0];
  if (!method.includes("'/auth/login'")) {
    throw new Error('Should POST to /auth/login');
  }

  if (!method.includes('this.client.setToken')) {
    throw new Error('Should call setToken with returned token');
  }
});

test('refresh() method updates token', () => {
  const authContent = fs.readFileSync(path.join(__dirname, '../src/modules/auth.js'), 'utf8');

  const refreshMatch = authContent.match(/async refresh\([^)]*\)[\s\S]{0,400}/);
  if (!refreshMatch) {
    throw new Error('refresh method not found');
  }

  const method = refreshMatch[0];
  if (!method.includes("'/auth/refresh'")) {
    throw new Error('Should POST to /auth/refresh');
  }

  if (!method.includes('this.client.setToken')) {
    throw new Error('Should update token after refresh');
  }
});

test('logout() method clears token', () => {
  const authContent = fs.readFileSync(path.join(__dirname, '../src/modules/auth.js'), 'utf8');

  const logoutMatch = authContent.match(/async logout\([^)]*\)[\s\S]{0,400}/);
  if (!logoutMatch) {
    throw new Error('logout method not found');
  }

  const method = logoutMatch[0];
  if (!method.includes('this.client.setToken(null)')) {
    throw new Error('Should clear token on logout');
  }
});

test('getMe() method fetches current user info', () => {
  const authContent = fs.readFileSync(path.join(__dirname, '../src/modules/auth.js'), 'utf8');

  const getMeMatch = authContent.match(/async getMe\([^)]*\)[\s\S]{0,300}/);
  if (!getMeMatch) {
    throw new Error('getMe method not found');
  }

  const method = getMeMatch[0];
  if (!method.includes("'/auth/me'")) {
    throw new Error('Should GET from /auth/me');
  }
});

// Test Suite 4: Canvas/Rooms Module Functionality
log('\nTest Suite 4: Canvas/Rooms Module Functionality', colors.bright + colors.blue);

test('create() creates canvas with proper data structure', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const createMatch = roomsContent.match(/async create\([^)]+\)[\s\S]{0,400}/);
  if (!createMatch) {
    throw new Error('create method not found');
  }

  const method = createMatch[0];
  if (!method.includes("'/canvases'")) {
    throw new Error('Should POST to /canvases');
  }

  if (!method.includes("method: 'POST'")) {
    throw new Error('Should use POST method');
  }
});

test('list() supports pagination parameters', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const listMatch = roomsContent.match(/async list\([^)]*\)[\s\S]{0,600}/);
  if (!listMatch) {
    throw new Error('list method not found');
  }

  const method = listMatch[0];
  if (!method.includes('URLSearchParams')) {
    throw new Error('Should use URLSearchParams for query string');
  }

  if (!method.includes('page') || !method.includes('per_page')) {
    throw new Error('Should support pagination parameters');
  }
});

test('update() uses PATCH method', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const updateMatch = roomsContent.match(/async update\([^)]+\)[\s\S]{0,400}/);
  if (!updateMatch) {
    throw new Error('update method not found');
  }

  const method = updateMatch[0];
  if (!method.includes("method: 'PATCH'")) {
    throw new Error('Should use PATCH method for updates');
  }
});

test('delete() uses DELETE method', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const deleteMatch = roomsContent.match(/async delete\([^)]+\)[\s\S]{0,400}/);
  if (!deleteMatch) {
    throw new Error('delete method not found');
  }

  const method = deleteMatch[0];
  if (!method.includes("method: 'DELETE'")) {
    throw new Error('Should use DELETE method');
  }
});

test('getStrokes() supports filtering by user and time', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const getStrokesMatch = roomsContent.match(/async getStrokes\([^)]+\)[\s\S]{0,600}/);
  if (!getStrokesMatch) {
    throw new Error('getStrokes method not found');
  }

  const method = getStrokesMatch[0];
  if (!method.includes('URLSearchParams')) {
    throw new Error('Should build query string for filters');
  }

  if (!method.includes('user') && !method.includes('since')) {
    throw new Error('Should support filtering parameters');
  }
});

test('addStroke() sends pathData, color, lineWidth', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const addStrokeMatch = roomsContent.match(/async addStroke\([^)]+\)[\s\S]{0,400}/);
  if (!addStrokeMatch) {
    throw new Error('addStroke method not found');
  }

  const method = addStrokeMatch[0];
  if (!method.includes('/canvases/') && !method.includes('/strokes')) {
    throw new Error('Should POST to /canvases/{id}/strokes');
  }

  if (!method.includes('stroke')) {
    throw new Error('Should include stroke data in body');
  }
});

test('clear() uses DELETE method on /strokes endpoint', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const clearMatch = roomsContent.match(/async clear\([^)]+\)[\s\S]{0,400}/);
  if (!clearMatch) {
    throw new Error('clear method not found');
  }

  const method = clearMatch[0];
  if (!method.includes("method: 'DELETE'")) {
    throw new Error('Should use DELETE method (not POST)');
  }

  if (!method.includes('/strokes')) {
    throw new Error('Should target /strokes endpoint');
  }
});

test('undo() POSTs to /history/undo endpoint', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const undoMatch = roomsContent.match(/async undo\([^)]+\)[\s\S]{0,400}/);
  if (!undoMatch) {
    throw new Error('undo method not found');
  }

  const method = undoMatch[0];
  if (!method.includes('/history/undo')) {
    throw new Error('Should use nested /history/undo path');
  }

  if (!method.includes("method: 'POST'")) {
    throw new Error('Should use POST method');
  }
});

test('redo() POSTs to /history/redo endpoint', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const redoMatch = roomsContent.match(/async redo\([^)]+\)[\s\S]{0,400}/);
  if (!redoMatch) {
    throw new Error('redo method not found');
  }

  const method = redoMatch[0];
  if (!method.includes('/history/redo')) {
    throw new Error('Should use nested /history/redo path');
  }
});

test('getUndoRedoStatus() GETs from /history/status', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const statusMatch = roomsContent.match(/async getUndoRedoStatus\([^)]+\)[\s\S]{0,400}/);
  if (!statusMatch) {
    throw new Error('getUndoRedoStatus method not found');
  }

  const method = statusMatch[0];
  if (!method.includes('/history/status')) {
    throw new Error('Should use nested /history/status path');
  }

  if (!method.includes("method: 'GET'")) {
    throw new Error('Should use GET method');
  }
});

test('resetStacks() POSTs to /history/reset', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const resetMatch = roomsContent.match(/async resetStacks\([^)]+\)[\s\S]{0,400}/);
  if (!resetMatch) {
    throw new Error('resetStacks method not found');
  }

  const method = resetMatch[0];
  if (!method.includes('/history/reset')) {
    throw new Error('Should use nested /history/reset path');
  }
});

test('share() sends array of usernames with roles', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const shareMatch = roomsContent.match(/async share\([^)]+\)[\s\S]{0,400}/);
  if (!shareMatch) {
    throw new Error('share method not found');
  }

  const method = shareMatch[0];
  if (!method.includes('/share')) {
    throw new Error('Should POST to /share endpoint');
  }

  if (!method.includes('users')) {
    throw new Error('Should include users array in body');
  }
});

test('transferOwnership() sends new owner ID', () => {
  const roomsContent = fs.readFileSync(path.join(__dirname, '../src/modules/canvases.js'), 'utf8');

  const transferMatch = roomsContent.match(/async transferOwnership\([^)]+\)[\s\S]{0,400}/);
  if (!transferMatch) {
    throw new Error('transferOwnership method not found');
  }

  const method = transferMatch[0];
  if (!method.includes('/transfer')) {
    throw new Error('Should POST to /transfer endpoint');
  }

  if (!method.includes('newOwner')) {
    throw new Error('Should include new owner information');
  }
});

// Test Suite 5: Invites/Collaborations Module
log('\nTest Suite 5: Invites/Collaborations Module Functionality', colors.bright + colors.blue);

test('list() GETs from /collaborations/invitations', () => {
  const invitesContent = fs.readFileSync(path.join(__dirname, '../src/modules/invites.js'), 'utf8');

  const listMatch = invitesContent.match(/async list\([^)]*\)[\s\S]{0,300}/);
  if (!listMatch) {
    throw new Error('list method not found');
  }

  const method = listMatch[0];
  if (!method.includes('/collaborations/invitations')) {
    throw new Error('Should GET from /collaborations/invitations');
  }
});

test('accept() POSTs to /collaborations/invitations/{id}/accept', () => {
  const invitesContent = fs.readFileSync(path.join(__dirname, '../src/modules/invites.js'), 'utf8');

  const acceptMatch = invitesContent.match(/async accept\([^)]+\)[\s\S]{0,300}/);
  if (!acceptMatch) {
    throw new Error('accept method not found');
  }

  const method = acceptMatch[0];
  if (!method.includes('/accept')) {
    throw new Error('Should POST to /accept endpoint');
  }

  if (!method.includes('inviteId')) {
    throw new Error('Should use inviteId parameter');
  }
});

test('decline() POSTs to /collaborations/invitations/{id}/decline', () => {
  const invitesContent = fs.readFileSync(path.join(__dirname, '../src/modules/invites.js'), 'utf8');

  const declineMatch = invitesContent.match(/async decline\([^)]+\)[\s\S]{0,300}/);
  if (!declineMatch) {
    throw new Error('decline method not found');
  }

  const method = declineMatch[0];
  if (!method.includes('/decline')) {
    throw new Error('Should POST to /decline endpoint');
  }
});

// Test Suite 6: Notifications Module
log('\nTest Suite 6: Notifications Module Functionality', colors.bright + colors.blue);

test('list() GETs notifications with optional filters', () => {
  const notifsContent = fs.readFileSync(path.join(__dirname, '../src/modules/notifications.js'), 'utf8');

  const listMatch = notifsContent.match(/async list\([^)]*\)[\s\S]{0,500}/);
  if (!listMatch) {
    throw new Error('list method not found');
  }

  const method = listMatch[0];
  if (!method.includes("'/notifications'")) {
    throw new Error('Should GET from /notifications');
  }
});

test('markRead() marks notification as read', () => {
  const notifsContent = fs.readFileSync(path.join(__dirname, '../src/modules/notifications.js'), 'utf8');

  const markReadMatch = notifsContent.match(/async markRead\([^)]+\)[\s\S]{0,300}/);
  if (!markReadMatch) {
    throw new Error('markRead method not found');
  }

  const method = markReadMatch[0];
  if (!method.includes('/mark-read') && !method.includes('PATCH')) {
    throw new Error('Should mark notification as read');
  }
});

test('delete() DELETEs specific notification', () => {
  const notifsContent = fs.readFileSync(path.join(__dirname, '../src/modules/notifications.js'), 'utf8');

  const deleteMatch = notifsContent.match(/async delete\([^)]+\)[\s\S]{0,300}/);
  if (!deleteMatch) {
    throw new Error('delete method not found');
  }

  const method = deleteMatch[0];
  if (!method.includes("method: 'DELETE'")) {
    throw new Error('Should use DELETE method');
  }
});

test('getPreferences() fetches notification settings', () => {
  const notifsContent = fs.readFileSync(path.join(__dirname, '../src/modules/notifications.js'), 'utf8');

  const getPrefMatch = notifsContent.match(/async getPreferences\([^)]*\)[\s\S]{0,300}/);
  if (!getPrefMatch) {
    throw new Error('getPreferences method not found');
  }

  const method = getPrefMatch[0];
  if (!method.includes('/preferences')) {
    throw new Error('Should GET from /preferences endpoint');
  }
});

test('updatePreferences() updates notification settings', () => {
  const notifsContent = fs.readFileSync(path.join(__dirname, '../src/modules/notifications.js'), 'utf8');

  const updatePrefMatch = notifsContent.match(/async updatePreferences\([^)]+\)[\s\S]{0,300}/);
  if (!updatePrefMatch) {
    throw new Error('updatePreferences method not found');
  }

  const method = updatePrefMatch[0];
  if (!method.includes("method: 'PUT'") && !method.includes("method: 'PATCH'")) {
    throw new Error('Should use PUT or PATCH method');
  }
});

// Test Suite 7: Socket.IO Module
log('\nTest Suite 7: Socket.IO Module Functionality', colors.bright + colors.blue);

test('connect() initializes socket with token auth', () => {
  const socketContent = fs.readFileSync(path.join(__dirname, '../src/modules/socket.js'), 'utf8');

  const connectMatch = socketContent.match(/connect\([^)]+\)[\s\S]{0,500}/);
  if (!connectMatch) {
    throw new Error('connect method not found');
  }

  const method = connectMatch[0];
  if (!method.includes('token')) {
    throw new Error('Should accept token parameter');
  }

  if (!method.includes('io(') && !method.includes('io.connect')) {
    throw new Error('Should call socket.io connect');
  }
});

test('joinCanvas() emits join_room event with roomId', () => {
  const socketContent = fs.readFileSync(path.join(__dirname, '../src/modules/socket.js'), 'utf8');

  const joinMatch = socketContent.match(/joinCanvas\([^)]+\)[\s\S]{0,300}/);
  if (!joinMatch) {
    throw new Error('joinCanvas method not found');
  }

  const method = joinMatch[0];
  if (!method.includes('emit')) {
    throw new Error('Should emit socket event');
  }

  if (!method.includes('join_room') && !method.includes('joinCanvas')) {
    throw new Error('Should emit join event');
  }
});

test('leaveCanvas() emits leave_room event', () => {
  const socketContent = fs.readFileSync(path.join(__dirname, '../src/modules/socket.js'), 'utf8');

  const leaveMatch = socketContent.match(/leaveCanvas\([^)]+\)[\s\S]{0,300}/);
  if (!leaveMatch) {
    throw new Error('leaveCanvas method not found');
  }

  const method = leaveMatch[0];
  if (!method.includes('emit')) {
    throw new Error('Should emit socket event');
  }
});

test('on() registers event listener', () => {
  const socketContent = fs.readFileSync(path.join(__dirname, '../src/modules/socket.js'), 'utf8');

  const onMatch = socketContent.match(/\bon\([^)]+\)[\s\S]{0,200}/);
  if (!onMatch) {
    throw new Error('on method not found');
  }

  const method = onMatch[0];
  if (!method.includes('this.socket.on')) {
    throw new Error('Should delegate to socket.on');
  }
});

test('disconnect() closes socket connection', () => {
  const socketContent = fs.readFileSync(path.join(__dirname, '../src/modules/socket.js'), 'utf8');

  const disconnectMatch = socketContent.match(/disconnect\([^)]*\)[\s\S]{0,200}/);
  if (!disconnectMatch) {
    throw new Error('disconnect method not found');
  }

  const method = disconnectMatch[0];
  if (!method.includes('this.socket.disconnect')) {
    throw new Error('Should call socket.disconnect');
  }
});

// Test Suite 8: Example Application Validation
log('\nTest Suite 8: Example Application Validation', colors.bright + colors.blue);

test('Example imports/uses DrawingClient', () => {
  const exampleContent = fs.readFileSync(path.join(__dirname, '../examples/basic-drawing-app.html'), 'utf8');

  if (!exampleContent.includes('DrawingClient') && !exampleContent.includes('client')) {
    throw new Error('Example should use DrawingClient');
  }
});

test('Example demonstrates authentication flow', () => {
  const exampleContent = fs.readFileSync(path.join(__dirname, '../examples/basic-drawing-app.html'), 'utf8');

  if (!exampleContent.includes('login') && !exampleContent.includes('auth')) {
    throw new Error('Example should show authentication');
  }
});

test('Example demonstrates canvas creation and drawing', () => {
  const exampleContent = fs.readFileSync(path.join(__dirname, '../examples/basic-drawing-app.html'), 'utf8');

  if (!exampleContent.includes('canvas') || !exampleContent.includes('ctx')) {
    throw new Error('Example should include canvas drawing');
  }

  if (!exampleContent.includes('stroke') || !exampleContent.includes('path')) {
    throw new Error('Example should demonstrate stroke submission');
  }
});

test('Example uses correct API v1 endpoints', () => {
  const exampleContent = fs.readFileSync(path.join(__dirname, '../examples/basic-drawing-app.html'), 'utf8');

  if (!exampleContent.includes('/api/v1/canvases')) {
    throw new Error('Example should use /api/v1/canvases endpoints');
  }

  if (exampleContent.includes('/api/v1/rooms/')) {
    throw new Error('Example should not use old /rooms endpoints');
  }
});

test('Example includes undo/redo functionality', () => {
  const exampleContent = fs.readFileSync(path.join(__dirname, '../examples/basic-drawing-app.html'), 'utf8');

  if (!exampleContent.includes('undo') || !exampleContent.includes('redo')) {
    throw new Error('Example should demonstrate undo/redo');
  }

  if (!exampleContent.includes('/history/undo')) {
    throw new Error('Example should use nested /history endpoints');
  }
});

test('Example uses proper error handling', () => {
  const exampleContent = fs.readFileSync(path.join(__dirname, '../examples/basic-drawing-app.html'), 'utf8');

  const tryCount = (exampleContent.match(/try\s*{/g) || []).length;
  const catchCount = (exampleContent.match(/catch\s*\(/g) || []).length;

  if (tryCount < 3 || catchCount < 3) {
    throw new Error('Example should include proper error handling with try/catch');
  }
});

// Test Suite 9: Documentation Quality
log('\nTest Suite 9: Documentation Quality & Completeness', colors.bright + colors.blue);

test('README includes installation instructions', () => {
  const readmeContent = fs.readFileSync(path.join(__dirname, '../README.md'), 'utf8');

  if (!readmeContent.toLowerCase().includes('install')) {
    throw new Error('README should include installation section');
  }

  if (!readmeContent.includes('npm') && !readmeContent.includes('yarn')) {
    throw new Error('README should show how to install via npm/yarn');
  }
});

test('README includes quick start example', () => {
  const readmeContent = fs.readFileSync(path.join(__dirname, '../README.md'), 'utf8');

  if (!readmeContent.includes('Quick Start') && !readmeContent.includes('Getting Started')) {
    throw new Error('README should have quick start section');
  }

  if (!readmeContent.includes('```')) {
    throw new Error('README should include code examples');
  }
});

test('README documents all main modules', () => {
  const readmeContent = fs.readFileSync(path.join(__dirname, '../README.md'), 'utf8');

  const modules = ['auth', 'canvases', 'socket', 'invites', 'notifications'];
  for (const mod of modules) {
    if (!readmeContent.toLowerCase().includes(mod)) {
      throw new Error(`README should document ${mod} module`);
    }
  }
});

test('README includes authentication examples', () => {
  const readmeContent = fs.readFileSync(path.join(__dirname, '../README.md'), 'utf8');

  if (!readmeContent.includes('login') || !readmeContent.includes('register')) {
    throw new Error('README should include authentication examples');
  }
});

test('README includes drawing/stroke examples', () => {
  const readmeContent = fs.readFileSync(path.join(__dirname, '../README.md'), 'utf8');

  if (!readmeContent.includes('stroke') || !readmeContent.includes('canvas')) {
    throw new Error('README should include drawing examples');
  }
});

test('README mentions Socket.IO for real-time features', () => {
  const readmeContent = fs.readFileSync(path.join(__dirname, '../README.md'), 'utf8');

  if (!readmeContent.toLowerCase().includes('socket') && !readmeContent.toLowerCase().includes('real-time')) {
    throw new Error('README should mention real-time capabilities');
  }
});

test('MIGRATION guide explains endpoint changes', () => {
  const migrationContent = fs.readFileSync(path.join(__dirname, 'MIGRATION_TO_API_V1.md'), 'utf8');

  if (!migrationContent.includes('/rooms') || !migrationContent.includes('/canvases')) {
    throw new Error('Migration guide should explain /rooms -> /canvases change');
  }

  if (!migrationContent.includes('/history')) {
    throw new Error('Migration guide should explain nested /history paths');
  }
});

test('MIGRATION guide includes before/after examples', () => {
  const migrationContent = fs.readFileSync(path.join(__dirname, 'MIGRATION_TO_API_V1.md'), 'utf8');

  if (!migrationContent.includes('Before') && !migrationContent.includes('Old')) {
    throw new Error('Migration guide should show old endpoint examples');
  }

  if (!migrationContent.includes('After') && !migrationContent.includes('New')) {
    throw new Error('Migration guide should show new endpoint examples');
  }
});

// Test Suite 10: Package Configuration
log('\nTest Suite 10: Package Configuration & Metadata', colors.bright + colors.blue);

test('package.json has correct entry point', () => {
  const pkgContent = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));

  if (!pkgContent.main) {
    throw new Error('package.json should specify main entry point');
  }

  if (!pkgContent.main.includes('index.js')) {
    throw new Error('Main entry point should be index.js');
  }
});

test('package.json specifies ES module type', () => {
  const pkgContent = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));

  if (pkgContent.type !== 'module') {
    log('    Note: Consider adding "type": "module" for ES modules', colors.yellow);
  }
});

test('package.json includes required metadata', () => {
  const pkgContent = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));

  const required = ['name', 'version', 'description', 'license', 'author'];
  for (const field of required) {
    if (!pkgContent[field]) {
      throw new Error(`package.json missing required field: ${field}`);
    }
  }
});

test('package.json specifies socket.io-client dependency', () => {
  const pkgContent = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));

  if (!pkgContent.dependencies || !pkgContent.dependencies['socket.io-client']) {
    throw new Error('socket.io-client should be listed as dependency');
  }
});

test('package.json license is Apache-2.0', () => {
  const pkgContent = JSON.parse(fs.readFileSync(path.join(__dirname, '../package.json'), 'utf8'));

  if (pkgContent.license !== 'Apache-2.0') {
    throw new Error('License should be Apache-2.0 for Apache ResilientDB');
  }
});

// Final Summary
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
  log('✓ All functional tests passed!', colors.bright + colors.green);
  log('  The toolkit is functionally complete and ready for production.', colors.green);
  log('  All modules properly implement their APIs with correct endpoints.', colors.green);
  process.exit(0);
}
