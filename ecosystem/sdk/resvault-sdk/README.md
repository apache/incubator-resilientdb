#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

# ResVault SDK
**License**: Apache-2.0

A lightweight SDK for integrating ResVault into your web applications. This SDK simplifies the interaction with the ResVault system, allowing you to send messages and listen for events in a secure and controlled manner.

## Features

- Simple API to send and receive messages via `postMessage`.
- Ability to add and remove custom message listeners.
- Designed for integration with ResVault-based applications.

## Installation

You can install the ResVault SDK via npm:

```bash
npm install resvault-sdk
```

## Usage

### Importing the SDK

After installing the SDK, you can import and use it in your JavaScript or TypeScript projects as follows:

```javascript
import ResVaultSDK from 'resvault-sdk';

const sdk = new ResVaultSDK();

// Sending a message to the content script
sdk.sendMessage({ type: 'login', direction: 'login' });

// Adding a message listener to receive messages
const handleMessage = (event) => {
  console.log('Received message:', event.data);
};
sdk.addMessageListener(handleMessage);

// Removing the message listener when it's no longer needed
sdk.removeMessageListener(handleMessage);
```

### API Reference

#### `new ResVaultSDK(targetOrigin?: string)`

- **targetOrigin**: _(optional)_ The target origin for `postMessage`. Defaults to `"*"`. You can specify this for more security.
  
Creates an instance of the `ResVaultSDK`.

#### `sendMessage(message: Message): void`

- **message**: The message object to send, which must include at least a `type` key.

Sends a message using `postMessage`.

#### `addMessageListener(handler: (event: MessageEvent) => void): void`

- **handler**: A function that will be called when a message is received. This function should accept a `MessageEvent` object as its parameter.

Adds a message listener for incoming messages.

#### `removeMessageListener(handler: (event: MessageEvent) => void): void`

- **handler**: The function that was previously registered as a listener.

Removes a message listener when it’s no longer needed.

## Build

To build the SDK from source:

```bash
npm run build
```

This will compile the TypeScript source code into JavaScript and generate the `dist` folder.

## Examples

If you're looking for a quick way to integrate this SDK with React or Vue projects, you can use the [create-resilient-app](https://www.npmjs.com/package/create-resilient-app) npm package. It helps you scaffold a new project that already includes the ResVault SDK integration.

### Steps to scaffold a project using `create-resilient-app`:

1. Run the following command with your chosen options (React/Vue, TypeScript/JavaScript):

    ```bash
    npx create-resilient-app --framework react --language typescript --name my-resvault-app
    ```

2. Navigate to your newly created project:

    ```bash
    cd my-resvault-app
    ```

3. Install the dependencies and start developing:

    ```bash
    npm install
    npm start
    ```

This will set up a complete React or Vue project (with TypeScript or JavaScript, depending on your choice) with the necessary setup to integrate the `ResVaultSDK`. You can explore the `Login` and `TransactionForm` components in the generated project to see how the SDK is integrated.

## License

This project is licensed under the terms of the Apache 2.0 License. See the [LICENSE](LICENSE) file for details.
