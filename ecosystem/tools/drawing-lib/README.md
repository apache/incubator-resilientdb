# Collaborative Drawing Toolkit

**License**: Apache-2.0

A universal JavaScript client library for building collaborative drawing applications with blockchain-backed storage. This toolkit provides a complete SDK for managing canvases, submitting drawing strokes, real-time collaboration, and integrating with ResilientDB's decentralized infrastructure.

## Features

- 🎨 **Drawing Operations**: Submit strokes, undo/redo, clear canvas
- 🔐 **Authentication**: JWT-based auth with secure canvas support
- 🏠 **Canvas Management**: Create, share, and manage collaborative drawing canvases
- ⚡ **Real-Time Collaboration**: Socket.IO-based live updates
- 🔗 **ResilientDB Integration**: Immutable stroke storage on blockchain
- 🛡️ **Secure Canvases**: Cryptographic signing for verified contributions
- 📦 **Modular Design**: Use only what you need
- 🔄 **Auto-Retry Logic**: Built-in request retry with exponential backoff

## Project Structure

```
drawing-toolkit/
├── src/                   # Source code
│   ├── index.js           # Main client export
│   └── modules/           # SDK modules (auth, canvases, socket, etc.)
├── examples/              # Example applications
├── tests/                 # Test suite
├── dist/                  # Built files (generated)
├── LICENSE                # Apache 2.0 License
├── README.md              # This file
└── package.json           # Package configuration
```

## Installation

```bash
npm install collaborative-drawing-toolkit
```

Or using yarn:

```bash
yarn add collaborative-drawing-toolkit
```

## Backend Requirements

**This toolkit is a client library and requires a compatible backend server to function.**

### Required Infrastructure

You must have the following services running and accessible:

1. **Backend API Server** - Handles authentication, authorization, and data management
2. **ResilientDB** - Decentralized blockchain database for immutable storage
3. **MongoDB** - Warm cache and queryable data replica
4. **Redis** - In-memory cache for real-time performance

### Backend Setup Example

This toolkit is designed to work with any backend that implements the required API endpoints. For a reference implementation, see compatible backend servers that support:

- JWT authentication (`/api/v1/auth/*`)
- Canvas management (`/api/v1/canvases/*`)
- Real-time collaboration via Socket.IO
- ResilientDB integration for blockchain storage

Example backend setup:
```bash
# Install backend dependencies
pip install flask flask-socketio redis pymongo resilientdb

# Configure environment variables
export MONGO_URI="mongodb://localhost:27017"
export REDIS_URL="redis://localhost:6379"
export RESILIENTDB_ENDPOINT="http://localhost:8000"

# Start backend server
python app.py
# Backend runs on http://localhost:10010 by default
```

## Quick Start

```javascript
import DrawingClient from 'collaborative-drawing-toolkit';

// Initialize the client (point to your backend server)
const client = new DrawingClient({
  baseUrl: 'http://localhost:10010',  // Your backend server URL
  apiVersion: 'v1'
});

// Authenticate
const { token, user } = await client.auth.login({
  username: 'alice',
  password: 'securePassword'
});

// Create a collaborative drawing canvas
const { canvas } = await client.canvases.create({
  name: 'My Artwork',
  type: 'public' // 'public', 'private', or 'secure'
});

// Submit a drawing stroke
await client.canvases.addStroke(canvas.id, {
  pathData: [
    { x: 100, y: 100 },
    { x: 200, y: 150 },
    { x: 300, y: 200 }
  ],
  color: '#FF0000',
  lineWidth: 3
});

// Real-time collaboration
client.socket.connect(token);
client.socket.joinCanvas(canvas.id);
client.socket.on('new_line', (stroke) => {
  console.log('New stroke from collaborator:', stroke);
  // Render the stroke on your canvas
});
```

## Architecture

This toolkit integrates with a backend server and ResilientDB infrastructure to provide:

1. **Immutable Storage**: All drawing strokes are stored in ResilientDB via GraphQL
2. **Caching Layer**: Redis for real-time performance, MongoDB for queryable replica
3. **Real-Time Updates**: Socket.IO for instant collaboration
4. **Authentication**: JWT-based with wallet support for secure operations

## API Reference

### Client Initialization

```javascript
const client = new DrawingClient({
  baseUrl: 'https://api.example.com',  // Required: Your backend server URL
  apiVersion: 'v1',                      // Optional: API version (default: 'v1')
  timeout: 30000,                        // Optional: Request timeout ms (default: 30000)
  retries: 3,                            // Optional: Retry attempts (default: 3)
  onTokenExpired: async () => {          // Optional: Token refresh handler
    // Return new token or throw error
    return await refreshToken();
  }
});
```

### Authentication

#### Register
```javascript
const { token, user } = await client.auth.register({
  username: 'alice',
  password: 'securePassword123',
  walletPubKey: 'optional_wallet_public_key' // For secure canvases
});
```

#### Login
```javascript
const { token, user } = await client.auth.login({
  username: 'alice',
  password: 'securePassword123'
});
```

#### Logout
```javascript
await client.auth.logout();
```

#### Get Current User
```javascript
const { user } = await client.auth.getMe();
```

### Canvas Management

#### Create Canvas
```javascript
const { canvas } = await client.canvases.create({
  name: 'Collaborative Canvas',
  type: 'public',  // 'public', 'private', or 'secure'
  description: 'Optional description'
});
```

#### List Canvases
```javascript
const { canvases, pagination } = await client.canvases.list({
  sortBy: 'createdAt',
  order: 'desc',
  page: 1,
  per_page: 20
});
```

#### Get Canvas Details
```javascript
const { canvas } = await client.canvases.get(canvasId);
```

#### Update Canvas
```javascript
const { canvas } = await client.canvases.update(canvasId, {
  name: 'New Name',
  description: 'Updated description',
  archived: false
});
```

#### Delete Canvas
```javascript
await client.canvases.delete(canvasId);
```

#### Share Canvas
```javascript
await client.canvases.share(canvasId, [
  { username: 'bob', role: 'editor' },
  { username: 'carol', role: 'viewer' }
]);
```

### Drawing Operations

#### Get Strokes
```javascript
const { strokes } = await client.canvases.getStrokes(canvasId, {
  since: timestamp,  // Optional: get strokes after this time
  until: timestamp   // Optional: get strokes before this time
});
```

#### Add Stroke
```javascript
await client.canvases.addStroke(canvasId, {
  pathData: [
    { x: 10, y: 20 },
    { x: 30, y: 40 },
    { x: 50, y: 60 }
  ],
  color: '#000000',
  lineWidth: 2,
  tool: 'pen',           // Optional: tool name
  signature: '...',       // Optional: for secure canvases
  signerPubKey: '...'     // Optional: for secure canvases
});
```

#### Undo
```javascript
await client.canvases.undo(canvasId);
```

#### Redo
```javascript
await client.canvases.redo(canvasId);
```

#### Clear Canvas
```javascript
await client.canvases.clear(canvasId);
```

### Collaboration & Invitations

#### List Pending Invitations
```javascript
const { invites } = await client.invites.list();
```

#### Accept an Invitation
```javascript
await client.invites.accept(inviteId);
```

#### Decline an Invitation
```javascript
await client.invites.decline(inviteId);
```

### Notifications

#### List Notifications
```javascript
const { notifications } = await client.notifications.list();
```

#### Mark Notification as Read
```javascript
await client.notifications.markRead(notificationId);
```

#### Delete Notification
```javascript
await client.notifications.delete(notificationId);
```

#### Clear All Notifications
```javascript
await client.notifications.clear();
```

#### Get Notification Preferences
```javascript
const { preferences } = await client.notifications.getPreferences();
```

#### Update Notification Preferences
```javascript
await client.notifications.updatePreferences({
  canvasInvites: true,
  mentions: true,
  canvasActivity: false
});
```

### Real-Time Collaboration

```javascript
// Connect to Socket.IO server
client.socket.connect(token);

// Join a room for real-time updates
client.socket.joinCanvas(canvasId);

// Listen for events
client.socket.on('new_line', (stroke) => {
  // New stroke added by collaborator
});

client.socket.on('undo_line', (data) => {
  // Stroke was undone
});

client.socket.on('redo_line', (data) => {
  // Stroke was redone
});

client.socket.on('clear_canvas', () => {
  // Canvas was cleared
});

client.socket.on('user_joined', (user) => {
  // User joined the room
});

client.socket.on('user_left', (user) => {
  // User left the room
});

// Leave room
client.socket.leaveCanvas(canvasId);

// Disconnect
client.socket.disconnect();
```

## Integration with ResilientDB

This toolkit is designed to work seamlessly with ResilientDB's blockchain infrastructure:

### Backend Integration

A compatible backend server should integrate:

1. **ResilientDB KV Service**: For blockchain storage
2. **GraphQL Service**: For transaction submission
3. **Redis**: For real-time caching
4. **MongoDB**: For queryable stroke replica
5. **Sync Service**: To mirror ResilientDB data to MongoDB

Example backend structure:
```
backend/
├── app.py              # Server application
├── routes/             # API endpoints
├── services/
│   ├── graphql_service.py  # ResilientDB GraphQL client
│   └── db.py               # Redis + MongoDB setup
└── middleware/
    └── auth.py         # JWT authentication
```

### Stroke Flow

1. **Client** → Submits stroke via SDK
2. **Backend** → Writes to ResilientDB (immutable)
3. **Backend** → Caches in Redis (fast reads)
4. **Backend** → Broadcasts via Socket.IO (real-time)
5. **Sync Service** → Mirrors to MongoDB (queryable)

## Examples

### Basic Drawing App

See [examples/basic-drawing-app.html](examples/basic-drawing-app.html) for a complete example.

```javascript
import DrawingClient from 'collaborative-drawing-toolkit';

class DrawingApp {
  constructor(canvasElement) {
    this.canvas = canvasElement;
    this.ctx = this.canvas.getContext('2d');
    this.client = new DrawingClient({
      baseUrl: 'http://localhost:10010'
    });
    this.currentCanvas = null;
    this.isDrawing = false;
    this.currentPath = [];
  }

  async init() {
    // Login
    const { token } = await this.client.auth.login({
      username: 'artist',
      password: 'password'
    });

    // Create or join canvas
    const { canvas } = await this.client.canvases.create({
      name: 'My Canvas',
      type: 'public'
    });
    this.currentCanvas = canvas;

    // Setup real-time collaboration
    this.client.socket.connect(token);
    this.client.socket.joinCanvas(canvas.id);
    this.client.socket.on('new_line', (stroke) => {
      this.drawStroke(stroke);
    });

    // Setup canvas events
    this.setupCanvasEvents();
  }

  setupCanvasEvents() {
    this.canvas.addEventListener('mousedown', (e) => {
      this.isDrawing = true;
      this.currentPath = [{ x: e.offsetX, y: e.offsetY }];
    });

    this.canvas.addEventListener('mousemove', (e) => {
      if (!this.isDrawing) return;
      this.currentPath.push({ x: e.offsetX, y: e.offsetY });
      this.drawLocalPath(this.currentPath);
    });

    this.canvas.addEventListener('mouseup', async (e) => {
      if (!this.isDrawing) return;
      this.isDrawing = false;
      
      // Submit to server
      await this.client.canvases.addStroke(this.currentRoom.id, {
        pathData: this.currentPath,
        color: '#000000',
        lineWidth: 2
      });
    });
  }

  drawStroke(stroke) {
    const { pathData, color, lineWidth } = stroke;
    this.ctx.strokeStyle = color;
    this.ctx.lineWidth = lineWidth;
    this.ctx.beginPath();
    this.ctx.moveTo(pathData[0].x, pathData[0].y);
    pathData.slice(1).forEach(point => {
      this.ctx.lineTo(point.x, point.y);
    });
    this.ctx.stroke();
  }

  drawLocalPath(pathData) {
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    this.drawStroke({ pathData, color: '#000000', lineWidth: 2 });
  }
}

// Usage
const canvas = document.getElementById('drawingCanvas');
const app = new DrawingApp(canvas);
app.init();
```

### Secure Room with Wallet Signing

```javascript
import DrawingClient from 'collaborative-drawing-toolkit';
import { signMessage } from './wallet-utils';

const client = new DrawingClient({
  baseUrl: 'http://localhost:10010'
});

// Register with wallet
await client.auth.register({
  username: 'secure_user',
  password: 'password',
  walletPubKey: myWalletPublicKey
});

// Create secure canvas
const { canvas } = await client.canvases.create({
  name: 'Secure Collaborative Canvas',
  type: 'secure'
});

// Submit signed stroke
const strokeData = {
  pathData: [{ x: 10, y: 20 }, { x: 30, y: 40 }],
  color: '#FF0000',
  lineWidth: 3
};

const signature = await signMessage(JSON.stringify(strokeData), myWalletPrivateKey);

await client.canvases.addStroke(canvas.id, {
  ...strokeData,
  signature: signature,
  signerPubKey: myWalletPublicKey
});
```

## Error Handling

```javascript
try {
  await client.canvases.create({ name: 'Test', type: 'public' });
} catch (error) {
  if (error.isAuthError()) {
    // 401: Token expired or invalid
    console.error('Please log in again');
  } else if (error.isValidationError()) {
    // 400: Invalid input
    const errors = error.getValidationErrors();
    console.error('Validation errors:', errors);
  } else if (error.status === 403) {
    // Forbidden
    console.error('Insufficient permissions');
  } else {
    // Other errors
    console.error('Error:', error.getUserMessage());
  }
}
```

## Configuration

### Environment Variables

When deploying a backend server for this toolkit:

```env
# MongoDB for queryable stroke replica
MONGO_ATLAS_URI=mongodb+srv://...

# ResilientDB credentials
SIGNER_PUBLIC_KEY=...
SIGNER_PRIVATE_KEY=...
RESILIENTDB_BASE_URI=https://crow.resilientdb.com
RESILIENTDB_GRAPHQL_URI=https://cloud.resilientdb.com/graphql

# Redis for real-time caching
REDIS_HOST=localhost
REDIS_PORT=6379
```

## Testing

```bash
npm test
```

## Contributing

Contributions are welcome! Please follow standard open-source contribution practices.

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Open a Pull Request

## License

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Links

- [ResilientDB](https://resilientdb.com)
- [Apache ResilientDB](https://github.com/apache/incubator-resilientdb)