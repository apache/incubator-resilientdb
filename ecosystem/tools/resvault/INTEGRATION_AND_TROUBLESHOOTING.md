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
# ResVault Integration and Troubleshooting

This document explains how ResVault connects to third-party web apps (e.g. Knobe) and how to fix the issue where the extension is detected but transaction approvals or login fail ("can't connect").

## Target workflow: Knobe + ResVault + ResDB-ORM + ResilientDB

Desired flow:

```
Knobe Web Form → ResVault (sign/approve) → ResDB ORM → ResDB GraphQL → ResilientDB
```

- The **Knobe** app logs interactions (hash, metadata, agent/session IDs) and uses ResDB for fingerprinting and provenance; the full file stays in KNOBE Desk (IndexedDB).
- **ResVault** should be the layer where the user authenticates and approves transactions so that writes are tied to a wallet address.
- **ResDB-ORM** structures the payload into a ResDB asset; **ResDB GraphQL** commits and queries the ledger.

## How ResVault connection actually works

ResVault uses two mechanisms that must both succeed for your app to "connect":

### 1. Content script ↔ page (postMessage)

- ResVault injects a **content script** ([`public/content.js`](public/content.js)) on all URLs (`<all_urls>` in [manifest](public/manifest.json)).
- Your app uses **ResVault SDK** (`resvault-sdk`) to send messages:
  - **Login**: `{ type: 'login', direction: 'login' }`
  - **Transaction**: `{ type: 'commit', direction: 'commit', amount, data, recipient }` or `{ type: 'custom', direction: 'custom', data, recipient, customMessage }`
- The content script listens on `window.addEventListener('message')` and only reacts when `event.source === window` and `event.data.direction` is `'login'`, `'commit'`, or `'custom'`.
- So: the extension is "detected" when the content script is present and the modal appears. Connection can still **fail** at the next step.

### 2. "Connected" state per domain + network

Before ResVault can sign or submit anything, it checks whether the **current page’s hostname** is **connected** to a **network** in the extension.

- **Storage keys**: `chrome.storage.local` holds:
  - `connectedNets`: map of **domain (hostname) → network name** (e.g. `localhost` → `ResilientDB Localnet`).
  - `keys`: nested map **domain → net →** encrypted key material (publicKey, privateKey, url, etc.).
- **Where this is set**: Only when the user, in the **ResVault extension popup** (Dashboard), clicks the **Connect** control (globe/site icon). That calls `chrome.runtime.sendMessage({ action: 'storeKeys', domain, net, publicKey, privateKey, url })`. The popup gets `domain` from the **currently active browser tab** via `chrome.tabs.query({ active: true, currentWindow: true })`.
- **Where it’s checked**: In the content script, when the user clicks **Authenticate** (login) or **Submit** (transaction), it calls `checkConnectionStatus()` which:
  - Reads `window.location.hostname` (e.g. `localhost`, `app.knobe.io`).
  - Looks up `connectedNets[hostname]` and `keys[hostname][net]`.
  - If either is missing, it sends `'error'` back to the page and does not proceed.

So: **the app tab’s hostname must have been “connected” from the ResVault popup while that tab was active.** If that step was never done, or the hostname doesn’t match, you get “detects plugin but can’t connect.”

## Why “detects plugin but can’t connect” happens

Common causes:

| Cause | What happens | Fix |
|-------|----------------|-----|
| **Site never connected from popup** | `connectedNets[your-hostname]` and `keys[your-hostname][net]` are missing. | Open ResVault popup **with the Knobe (or your) app tab active**, select the correct network, then click the **Connect** (globe) control. |
| **Wrong tab active when connecting** | Popup stored keys for `twitter.com` (or another tab) instead of your app’s hostname. | Connect again with **your app’s tab** in focus. |
| **Hostname mismatch** | App is `http://127.0.0.1:3000` but extension stored under `localhost` (or vice versa). `getBaseDomain()` uses `urlObj.hostname` only. | Use one hostname consistently (e.g. always `localhost` or always `127.0.0.1`) when opening the app and when connecting from the popup. |
| **Not logged in to ResVault** | No keypair in the extension; `storeKeys` has nothing to store. | Log in (or create an account) inside ResVault first, then connect the site. |
| **Wrong network selected** | Stored net is e.g. Mainnet but your backend uses Localnet. | In the popup, select the same network your app/ORM/GraphQL use, then (re)connect. |
| **Content script not in same context** | If your “form” runs inside an iframe from another origin, the content script’s `window` may not be the iframe’s window, so `event.source === window` can fail. | Prefer having the ResVault SDK and form on the main page’s origin so the content script and page share the same top-level `window`. |

## Step-by-step: Getting Knobe (or any app) to connect

1. **Install ResVault**  
   Load unpacked from the extension’s build folder (e.g. `ecosystem/tools/resvault/build` or the built output you use).

2. **ResVault account**  
   Open the extension, sign up or log in, and ensure you have at least one keypair and the correct network (Mainnet / Localnet / Custom URL) configured.

3. **Open your app**  
   Navigate to your Knobe app in the same browser (e.g. `http://localhost:3000` or your deployed URL). Keep this tab **active**.

4. **Connect the site in ResVault**  
   - Open the ResVault extension popup (click the icon).  
   - Confirm the **network** (e.g. ResilientDB Localnet) matches your ResDB GraphQL backend.  
   - Click the **Connect** control (globe/site icon) so that the current tab’s hostname is stored with this network.  
   - You should see the connection state turn “Connected” for that site.

5. **Use the app**  
   In your app, trigger login or a transaction. The ResVault modal should appear; after you click Authenticate or Submit, the content script will find `connectedNets[hostname]` and `keys[hostname][net]` and proceed.

## SDK usage (reference)

Your app should:

- Use **ResVault SDK** to send the same message shapes the content script expects:
  - Login: `sdk.sendMessage({ type: 'login', direction: 'login' });`
  - Commit: `sdk.sendMessage({ type: 'commit', direction: 'commit', amount, data, recipient });`
  - Custom: `sdk.sendMessage({ type: 'custom', direction: 'custom', data, recipient, customMessage });`
- Listen for responses: `sdk.addMessageListener((event) => { ... })` and check `event.data.type === 'FROM_CONTENT_SCRIPT'` and `event.data.data` (success payload or `'error'`).

See [ResVault SDK README](../../sdk/resvault-sdk/README.md) and the [create-resilient-app](https://www.npmjs.com/package/create-resilient-app) templates (e.g. `ecosystem/tools/create-resilient-app/templates/react-ts-resvault-sdk`) for full examples.

## Where this is implemented in the repo

- **Content script (message handling, connection check)**: [`ecosystem/tools/resvault/public/content.js`](public/content.js)  
  - `checkConnectionStatus()` (domain + net), `handleLoginOperation`, `handleCommitOperation`, `handleCustomOperation`, and submission to background.
- **Background (storage, GraphQL)**: [`ecosystem/tools/resvault/public/background.js`](public/background.js)  
  - `storeKeys` / `disconnectKeys` (writes `keys`, `connectedNets`), `submitTransaction`, `submitLoginTransaction`.
- **Popup (connect UI)**: [`ecosystem/tools/resvault/src/pages/Dashboard.jsx`](src/pages/Dashboard.jsx)  
  - `domain` from active tab, `toggleConnection` → `storeKeys` / `disconnectKeys`.
- **Manifest**: [`ecosystem/tools/resvault/public/manifest.json`](public/manifest.json)  
  - `content_scripts` with `matches: ["<all_urls>"]`, `js: ["content.js"]`.

## Summary

- **Detection** = content script loaded and modal shown when your app sends `direction: 'login'` / `'commit'` / `'custom'`.
- **Connection** = that hostname has been connected from the ResVault popup for the chosen network, and keys exist for that domain+net.  
If the extension is detected but approvals fail, almost always the hostname was never connected (or connected under a different hostname/network). Connect the site from the popup with the app tab active and the correct network selected, then retry.
