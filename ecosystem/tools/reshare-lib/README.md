<!--
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing,
 software distributed under the License is distributed on an
 "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND, either express or implied.  See the License for the
 specific language governing permissions and limitations
 under the License.
-->

# ResShare Toolkit

**License**: Apache-2.0

Universal JavaScript toolkit for ResShare file-sharing APIs. This toolkit provides a modular client for authentication, file and folder operations, and share management.

## Features

- Authentication helpers (`signup`, `login`, `logout`, `status`, `deleteUser`)
- File operations (`createFolder`, `upload`, `deleteItem`, `download`, `downloadZip`)
- Share operations (`share`, `list`, `remove`)
- Cookie-aware sessions for Node.js and browser clients
- Retry and timeout handling with normalized API errors

## Project Structure

```
reshare-lib/
├── src/
│   ├── index.js
│   └── modules/
│       ├── auth.js
│       ├── files.js
│       └── shares.js
├── examples/
│   ├── basic-usage.js
│   └── advanced-demo.js
├── tests/
│   └── unit/
├── README.md
├── jest.config.cjs
└── package.json
```

## Installation

This toolkit is part of the Apache ResilientDB repository and is not published as a standalone package yet.

```bash
# From repository root
npm install ./ecosystem/tools/reshare-lib
```

## Quick Start

```javascript
import ResShareToolkitClient from 'resshare-toolkit';

const client = new ResShareToolkitClient({
  baseUrl: 'http://localhost:5000'
});

await client.auth.login({ username: 'alice', password: 'secret' });
await client.files.createFolder('docs');
```

## API Overview

### Client Initialization

```javascript
const client = new ResShareToolkitClient({
  baseUrl: 'http://localhost:5000',
  apiPrefix: '',                 // Optional path prefix (for example: /api/v1)
  timeout: 30000,                // Request timeout in milliseconds
  retries: 2,                    // Request retries
  credentials: 'include',        // Fetch credentials mode
  onAuthExpired: async () => ''  // Optional token refresh callback
});
```

### Auth Module

- `client.auth.login({ username, password })`
- `client.auth.signup({ username, password })`
- `client.auth.logout()`
- `client.auth.status()`
- `client.auth.deleteUser({ password })`

### Files Module

- `client.files.createFolder(path)`
- `client.files.upload({ file, path, skipProcessing, filename })`
- `client.files.deleteItem({ path, deleteInRoot })`
- `client.files.download({ path, isShared })`
- `client.files.downloadZip({ path, isShared })`

### Shares Module

- `client.shares.share({ target, path, node })`
- `client.shares.list()`
- `client.shares.remove({ combinedPath, fromUser, path })`

## Error Handling

All failed requests throw `ApiError` with:

- `status`: HTTP status code
- `body`: parsed response payload

```javascript
import ResShareToolkitClient, { ApiError } from 'resshare-toolkit';

const client = new ResShareToolkitClient({ baseUrl: 'http://localhost:5000' });

try {
  await client.files.createFolder('docs');
} catch (error) {
  if (error instanceof ApiError) {
    console.error(error.status, error.getUserMessage());
  }
}
```

## Examples

```bash
node ecosystem/tools/reshare-lib/examples/basic-usage.js
node ecosystem/tools/reshare-lib/examples/advanced-demo.js
```

`advanced-demo.js` writes outputs to `ecosystem/tools/reshare-lib/examples/out/` during execution.

## Testing

```bash
cd ecosystem/tools/reshare-lib
npm test
```

## Backend Requirements

The toolkit expects a ResShare-compatible backend exposing endpoints used by:

- `/login`, `/signup`, `/logout`, `/auth-status`, `/delete-user`
- `/create-folder`, `/upload`, `/delete`, `/download`, `/download-zip`
- `/share`, `/shared`

Configure `baseUrl` and optional `apiPrefix` to match your deployment.

## License

Licensed to the Apache Software Foundation (ASF) under the Apache License, Version 2.0.
