/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*/


let faviconUrls = {};

// Helper functions
function arrayBufferToBase64(buffer) {
    let binary = '';
    const bytes = new Uint8Array(buffer);
    bytes.forEach(b => binary += String.fromCharCode(b));
    return btoa(binary);
}

function base64ToArrayBuffer(base64) {
    const binary = atob(base64);
    const len = binary.length;
    const bytes = new Uint8Array(len);
    for (let i = 0; i < len; i++) {
        bytes[i] = binary.charCodeAt(i);
    }
    return bytes.buffer;
}

function uint8ArrayToBase64(uint8Array) {
    return arrayBufferToBase64(uint8Array.buffer);
}

function base64ToUint8Array(base64) {
    const arrayBuffer = base64ToArrayBuffer(base64);
    return new Uint8Array(arrayBuffer);
}

async function encryptData(data, key) {
    const iv = crypto.getRandomValues(new Uint8Array(12));
    const encoded = new TextEncoder().encode(data);
    const encrypted = await crypto.subtle.encrypt(
        {
            name: 'AES-GCM',
            iv: iv,
        },
        key,
        encoded
    );

    // Convert encrypted data and iv to Base64 strings
    const ciphertextBase64 = arrayBufferToBase64(encrypted);
    const ivBase64 = uint8ArrayToBase64(iv);

    return { ciphertext: ciphertextBase64, iv: ivBase64 };
}

async function decryptData(ciphertextBase64, ivBase64, key) {
    const ciphertext = base64ToArrayBuffer(ciphertextBase64);
    const iv = base64ToUint8Array(ivBase64);

    const decrypted = await crypto.subtle.decrypt(
        {
            name: 'AES-GCM',
            iv: iv,
        },
        key,
        ciphertext
    );
    const dec = new TextDecoder();
    return dec.decode(decrypted);
}

// Function to get full hostname from URL
function getBaseDomain(url) {
    try {
        const urlObj = new URL(url);
        return urlObj.hostname;
    } catch (error) {
        console.error('Invalid URL:', url);
        return '';
    }
}

// Function to escape special characters in GraphQL strings
function escapeGraphQLString(str) {
    if (typeof str !== 'string') {
        return '';
    }
    return str.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
}

// Function to get the favicon URL when a tab is updated
function updateFaviconUrl(tabId, changeInfo, tab) {
    if (changeInfo.status === 'complete' && tab.url) {
        const domain = getBaseDomain(tab.url);
        if (domain && tab.favIconUrl) {
            faviconUrls[domain] = tab.favIconUrl;

            // Also store in chrome.storage.local
            chrome.storage.local.get(['faviconUrls'], (result) => {
                let storedFavicons = result.faviconUrls || {};
                storedFavicons[domain] = tab.favIconUrl;
                chrome.storage.local.set({ faviconUrls: storedFavicons });
            });
        }
    }
}

// Add the listener for tab updates
chrome.tabs.onUpdated.addListener(updateFaviconUrl);

// Function to generate UUID v4
function generateUUID() {
    // Public Domain/MIT
    return ([1e7]+-1e3+-4e3+-8e3+-1e11).replace(/[018]/g, c =>
      (c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> c / 4).toString(16)
    );
}

chrome.runtime.onMessage.addListener(function (request, sender, sendResponse) {
    if (request.action === "storeKeys") {
        (async function() {
            try {
                // Generate new key material and encrypt the new keys
                const keyMaterial = await crypto.subtle.generateKey(
                    { name: 'AES-GCM', length: 256 },
                    true,
                    ['encrypt', 'decrypt']
                );

                const encryptedPublicKey = await encryptData(request.publicKey, keyMaterial);
                const encryptedPrivateKey = await encryptData(request.privateKey, keyMaterial);
                const encryptedUrl = await encryptData(request.url, keyMaterial);

                // Export the key material to JWK format
                const exportedKey = await crypto.subtle.exportKey('jwk', keyMaterial);

                // Store the encrypted keys per domain and net
                chrome.storage.local.get(['keys', 'connectedNets'], (result) => {
                    let keys = result.keys || {};
                    let connectedNets = result.connectedNets || {};

                    if (!keys[request.domain]) {
                        keys[request.domain] = {};
                    }

                    keys[request.domain][request.net] = {
                        publicKey: encryptedPublicKey,
                        privateKey: encryptedPrivateKey,
                        url: encryptedUrl,
                        exportedKey: exportedKey, // Store the exported key
                    };

                    connectedNets[request.domain] = request.net;

                    console.log('Storing keys for domain:', request.domain);
                    console.log('Storing net:', request.net);
                    console.log('ConnectedNets after storing:', connectedNets);

                    chrome.storage.local.set({ keys, connectedNets }, () => {
                        console.log('Keys stored for domain:', request.domain, 'and net:', request.net);
                        sendResponse({ success: true });
                    });
                });
            } catch (error) {
                console.error('Error in storeKeys:', error);
                sendResponse({ success: false, error: error.message });
            }
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === "disconnectKeys") {
        (async function() {
            // Remove the keys for the domain and net
            chrome.storage.local.get(['keys', 'connectedNets'], (result) => {
                let keys = result.keys || {};
                let connectedNets = result.connectedNets || {};
                if (keys[request.domain] && keys[request.domain][request.net]) {
                    delete keys[request.domain][request.net];
                    if (Object.keys(keys[request.domain]).length === 0) {
                        delete keys[request.domain];
                    }
                    if (connectedNets[request.domain] === request.net) {
                        delete connectedNets[request.domain];
                    }
                    chrome.storage.local.set({ keys, connectedNets }, () => {
                        console.log('Keys disconnected for domain:', request.domain, 'and net:', request.net);
                        console.log('ConnectedNets after disconnecting:', connectedNets);
                        sendResponse({ success: true });
                    });
                } else {
                    sendResponse({ success: false, error: 'No keys found to disconnect' });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === "requestData") {
        (async function() {
            chrome.storage.local.get(['keys', 'faviconUrls'], async function (result) {
                const keys = result.keys || {};
                const storedFavicons = result.faviconUrls || {};
                const faviconUrlForDomain = storedFavicons[request.domain] || '';

                if (keys[request.domain] && keys[request.domain][request.net]) {
                    const { publicKey, privateKey, url, exportedKey } = keys[request.domain][request.net];

                    try {
                        // Import the key material from JWK format
                        const keyMaterial = await crypto.subtle.importKey(
                            'jwk',
                            exportedKey,
                            { name: 'AES-GCM' },
                            true,
                            ['encrypt', 'decrypt']
                        );

                        const decryptedPublicKey = await decryptData(publicKey.ciphertext, publicKey.iv, keyMaterial);
                        const decryptedPrivateKey = await decryptData(privateKey.ciphertext, privateKey.iv, keyMaterial);
                        const decryptedUrl = await decryptData(url.ciphertext, url.iv, keyMaterial);

                        sendResponse({
                            publicKey: decryptedPublicKey,
                            privateKey: decryptedPrivateKey,
                            url: decryptedUrl,
                            faviconUrl: faviconUrlForDomain
                        });
                    } catch (error) {
                        console.error('Error decrypting data:', error);
                        sendResponse({ error: "Failed to decrypt data", faviconUrl: faviconUrlForDomain });
                    }
                } else {
                    sendResponse({ error: "No keys found for domain and net", faviconUrl: faviconUrlForDomain });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === 'getConnectedNet') {
        (async function() {
            chrome.storage.local.get(['connectedNets'], function (result) {
                const connectedNets = result.connectedNets || {};
                const net = connectedNets[request.domain];
                if (net) {
                    sendResponse({ net: net });
                } else {
                    sendResponse({ error: 'No connected net for domain' });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === 'submitTransactionFromDashboard') {
        (async function() {
            // Retrieve the necessary data
            const transactionData = request.transactionData;
            const domain = request.domain;
            const net = request.net;

            // Validate transactionData
            if (!transactionData || !transactionData.asset || !transactionData.recipientAddress || !transactionData.amount) {
                sendResponse({ success: false, error: 'Invalid transaction data.' });
                return;
            }

            // Retrieve the signer's keys and URL from storage
            chrome.storage.local.get(['keys', 'connectedNets'], async function (result) {
                const keys = result.keys || {};
                const connectedNets = result.connectedNets || {};

                if (!connectedNets[domain] || connectedNets[domain] !== net) {
                    sendResponse({ success: false, error: 'Not connected to the specified net for this domain.' });
                    return;
                }

                if (!keys[domain] || !keys[domain][net]) {
                    sendResponse({ success: false, error: 'Keys not found for the specified domain and net.' });
                    return;
                }

                const { publicKey, privateKey, url, exportedKey } = keys[domain][net];

                try {
                    // Import the key material from JWK format
                    const keyMaterial = await crypto.subtle.importKey(
                        'jwk',
                        exportedKey,
                        { name: 'AES-GCM',
                        },
                        true,
                        ['encrypt', 'decrypt']
                    );

                    const decryptedPublicKey = await decryptData(publicKey.ciphertext, publicKey.iv, keyMaterial);
                    const decryptedPrivateKey = await decryptData(privateKey.ciphertext, privateKey.iv, keyMaterial);
                    const decryptedUrl = await decryptData(url.ciphertext, url.iv, keyMaterial);

                    // Prepare the asset data
                    const assetData = JSON.stringify(transactionData.asset);

                    // Construct the GraphQL mutation
                    const mutation = `
                        mutation {
                            postTransaction(data: {
                                operation: "CREATE",
                                amount: ${parseInt(transactionData.amount)},
                                signerPublicKey: "${escapeGraphQLString(decryptedPublicKey)}",
                                signerPrivateKey: "${escapeGraphQLString(decryptedPrivateKey)}",
                                recipientPublicKey: "${escapeGraphQLString(transactionData.recipientAddress)}",
                                asset: """${assetData}"""
                            }) {
                                id
                            }
                        }
                    `;

                    // Send the mutation
                    const response = await fetch(decryptedUrl, {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json',
                        },
                        body: JSON.stringify({ query: mutation }),
                    });

                    if (!response.ok) {
                        throw new Error(`Network response was not ok: ${response.statusText}`);
                    }

                    const resultData = await response.json();
                    if (resultData.errors) {
                        console.error('GraphQL errors:', resultData.errors);
                        sendResponse({ success: false, error: 'GraphQL errors occurred.', errors: resultData.errors });
                    } else {
                        console.log('Transaction submitted successfully:', resultData.data);
                        sendResponse({ success: true, data: resultData.data });
                    }

                } catch (error) {
                    console.error('Error submitting transaction:', error);
                    sendResponse({ success: false, error: error.message });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === 'submitTransaction') {
        // Added the missing submitTransaction handler

        (async function() {
            console.log('Handling submitTransaction action');
            console.log('Sender:', sender);
            let senderUrl = null;
            if (sender.tab && sender.tab.url) {
                senderUrl = sender.tab.url;
            } else if (sender.url) {
                senderUrl = sender.url;
            } else if (sender.origin) {
                senderUrl = sender.origin;
            } else {
                console.error('Sender URL is undefined');
                sendResponse({ success: false, error: 'Cannot determine sender URL' });
                return;
            }
            console.log('Sender URL:', senderUrl);

            const domain = getBaseDomain(senderUrl);
            console.log('Domain:', domain);

            chrome.storage.local.get(['keys', 'connectedNets'], async function (result) {
                const keys = result.keys || {};
                const connectedNets = result.connectedNets || {};
                console.log('ConnectedNets:', connectedNets);
                const net = connectedNets[domain];
                console.log('Net for domain:', domain, 'is', net);

                if (keys[domain] && keys[domain][net]) {
                    const { publicKey, privateKey, url, exportedKey } = keys[domain][net];

                    try {
                        // Import the key material from JWK format
                        const keyMaterial = await crypto.subtle.importKey(
                            'jwk',
                            exportedKey,
                            { name: 'AES-GCM' },
                            true,
                            ['encrypt', 'decrypt']
                        );

                        const decryptedPublicKey = await decryptData(publicKey.ciphertext, publicKey.iv, keyMaterial);
                        const decryptedPrivateKey = await decryptData(privateKey.ciphertext, privateKey.iv, keyMaterial);
                        const decryptedUrl = await decryptData(url.ciphertext, url.iv, keyMaterial);

                        // Check if required fields are defined
                        if (!decryptedPublicKey || !decryptedPrivateKey || !request.recipient) {
                            console.error('Missing required fields for transaction submission');
                            sendResponse({ success: false, error: 'Missing required fields for transaction' });
                            return;
                        }

                        // Prepare asset data as a JSON string
                        const assetData = JSON.stringify({
                            data: request.data || {}
                        });

                        // Construct the GraphQL mutation
                        const mutation = `
                            mutation {
                                postTransaction(data: {
                                    operation: "CREATE",
                                    amount: ${parseInt(request.amount)},
                                    signerPublicKey: "${escapeGraphQLString(decryptedPublicKey)}",
                                    signerPrivateKey: "${escapeGraphQLString(decryptedPrivateKey)}",
                                    recipientPublicKey: "${escapeGraphQLString(request.recipient)}",
                                    asset: """${assetData}"""
                                }) {
                                    id
                                }
                            }
                        `;

                        const response = await fetch(decryptedUrl, {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ query: mutation }),
                        });

                        if (!response.ok) {
                            throw new Error(`Network response was not ok: ${response.statusText}`);
                        }

                        const resultData = await response.json();
                        if (resultData.errors) {
                            console.error('GraphQL errors:', resultData.errors);
                            sendResponse({ success: false, errors: resultData.errors });
                        } else {
                            console.log('Transaction submitted successfully:', resultData.data);
                            sendResponse({ success: true, data: resultData.data });
                        }
                    } catch (error) {
                        console.error('Error submitting transaction:', error);
                        sendResponse({ success: false, error: error.message });
                    }
                } else {
                    console.error('No keys found for domain:', domain, 'and net:', net);
                    console.log('Available keys:', keys);
                    sendResponse({ error: "No keys found for domain and net" });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === 'submitLoginTransaction') {
        (async function() {
            console.log('Handling submitLoginTransaction action');
            console.log('Sender:', sender);
            let senderUrl = null;
            if (sender.tab && sender.tab.url) {
                senderUrl = sender.tab.url;
            } else if (sender.url) {
                senderUrl = sender.url;
            } else if (sender.origin) {
                senderUrl = sender.origin;
            } else {
                console.error('Sender URL is undefined');
                sendResponse({ success: false, error: 'Cannot determine sender URL' });
                return;
            }
            console.log('Sender URL:', senderUrl);

            const domain = getBaseDomain(senderUrl);
            console.log('Domain:', domain);

            chrome.storage.local.get(['keys', 'connectedNets'], async function (result) {
                const keys = result.keys || {};
                const connectedNets = result.connectedNets || {};
                console.log('ConnectedNets:', connectedNets);
                const net = connectedNets[domain];
                console.log('Net for domain:', domain, 'is', net);

                if (keys[domain] && keys[domain][net]) {
                    const { publicKey, privateKey, url, exportedKey } = keys[domain][net];

                    try {
                        // Import the key material from JWK format
                        const keyMaterial = await crypto.subtle.importKey(
                            'jwk',
                            exportedKey,
                            { name: 'AES-GCM' },
                            true,
                            ['encrypt', 'decrypt']
                        );

                        const decryptedPublicKey = await decryptData(publicKey.ciphertext, publicKey.iv, keyMaterial);
                        const decryptedPrivateKey = await decryptData(privateKey.ciphertext, privateKey.iv, keyMaterial);
                        const decryptedUrl = await decryptData(url.ciphertext, url.iv, keyMaterial);

                        // Prepare asset data with current timestamp and unique login_transaction_id
                        const currentTimestamp = Math.floor(Date.now() / 1000);

                        let loginTransactionId = '';
                        if (crypto.randomUUID) {
                            loginTransactionId = crypto.randomUUID().replace(/[^a-zA-Z0-9]/g, '');
                        } else {
                            loginTransactionId = generateUUID().replace(/[^a-zA-Z0-9]/g, '');
                        }

                        const assetData = JSON.stringify({
                            data: {
                                login_timestamp: currentTimestamp,
                                login_transaction_id: loginTransactionId
                            }
                        });

                        // Construct the GraphQL mutation
                        const mutation = `
                            mutation {
                                postTransaction(data: {
                                    operation: "CREATE"
                                    amount: 1
                                    signerPublicKey: "${escapeGraphQLString(decryptedPublicKey)}"
                                    signerPrivateKey: "${escapeGraphQLString(decryptedPrivateKey)}"
                                    recipientPublicKey: "${escapeGraphQLString(decryptedPublicKey)}"
                                    asset: """${assetData}"""
                                }) {
                                    id
                                }
                            }
                        `;

                        const response = await fetch(decryptedUrl, {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ query: mutation }),
                        });

                        if (!response.ok) {
                            throw new Error(`Network response was not ok: ${response.statusText}`);
                        }

                        const resultData = await response.json();
                        if (resultData.errors) {
                            console.error('GraphQL errors:', resultData.errors);
                            sendResponse({ success: false, errors: resultData.errors });
                        } else {
                            console.log('Login transaction submitted successfully:', resultData.data);
                            sendResponse({ success: true, data: resultData.data });
                        }
                    } catch (error) {
                        console.error('Error submitting login transaction:', error);
                        sendResponse({ success: false, error: error.message });
                    }
                } else {
                    console.error('No keys found for domain:', domain, 'and net:', net);
                    console.log('Available keys:', keys);
                    sendResponse({ error: "No keys found for domain and net" });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    } else if (request.action === 'deployContractChain') {
        // Handler for deploying contract chain
        (async function() {
            const domain = request.domain;
            const net = request.net;
            const ownerAddress = request.ownerAddress;
            const soliditySource = request.soliditySource;
            const deployConfig = request.deployConfig; // Contains arguments and contract_name

            // Retrieve the signer's keys and URL from storage
            chrome.storage.local.get(['keys', 'connectedNets'], async function (result) {
                const keys = result.keys || {};
                const connectedNets = result.connectedNets || {};

                if (!connectedNets[domain] || connectedNets[domain] !== net) {
                    sendResponse({ success: false, error: 'Not connected to the specified net for this domain.' });
                    return;
                }

                if (!keys[domain] || !keys[domain][net]) {
                    sendResponse({ success: false, error: 'Keys not found for the specified domain and net.' });
                    return;
                }

                const { url, exportedKey } = keys[domain][net];

                try {
                    // Import the key material from JWK format
                    const keyMaterial = await crypto.subtle.importKey(
                        'jwk',
                        exportedKey,
                        { name: 'AES-GCM',
                        },
                        true,
                        ['encrypt', 'decrypt']
                    );

                    const decryptedUrl = await decryptData(url.ciphertext, url.iv, keyMaterial);

                    // 1. Perform addAddress mutation
                    const addAddressMutation = `
                    mutation {
                      addAddress(
                        config: "5 127.0.0.1 10005",
                        address: "${escapeGraphQLString(ownerAddress)}",
                        type: "data"
                      )
                    }
                    `;

                    const addAddressResponse = await fetch(decryptedUrl, {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json',
                        },
                        body: JSON.stringify({ query: addAddressMutation }),
                    });

                    if (!addAddressResponse.ok) {
                        throw new Error(`Network response was not ok: ${addAddressResponse.statusText}`);
                    }

                    const addAddressResult = await addAddressResponse.json();
                    if (addAddressResult.errors) {
                        console.error('GraphQL errors in addAddress:', addAddressResult.errors);
                        sendResponse({ success: false, error: 'Error in addAddress mutation.', errors: addAddressResult.errors });
                        return;
                    }

                    // Check if addAddress was successful
                    if (addAddressResult.data && addAddressResult.data.addAddress === "Address added successfully") {
                        // 2. Perform compileContract mutation
                        const escapedSoliditySource = escapeGraphQLString(soliditySource);

                        const compileContractMutation = `
                        mutation {
                          compileContract(
                            source: """${escapedSoliditySource}""",
                            type: "data"
                        )
                        }
                        `;

                        const compileContractResponse = await fetch(decryptedUrl, {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ query: compileContractMutation }),
                        });

                        if (!compileContractResponse.ok) {
                            throw new Error(`Network response was not ok: ${compileContractResponse.statusText}`);
                        }

                        const compileContractResult = await compileContractResponse.json();
                        if (compileContractResult.errors) {
                            console.error('GraphQL errors in compileContract:', compileContractResult.errors);
                            sendResponse({ success: false, error: 'Error in compileContract mutation.', errors: compileContractResult.errors });
                            return;
                        }

                        // Extract the contract filename
                        const contractFilename = compileContractResult.data.compileContract;
                        if (!contractFilename) {
                            sendResponse({ success: false, error: 'Failed to compile contract.' });
                            return;
                        }

                        // 3. Perform deployContract mutation
                        const { arguments: args, contract_name } = deployConfig;
                        const deployContractMutation = `
                        mutation {
                          deployContract(
                            config: "5 127.0.0.1 10005",
                            contract: "${escapeGraphQLString(contractFilename)}",
                            name: "/tmp/${escapeGraphQLString(contractFilename.replace('.json', '.sol'))}:${escapeGraphQLString(contract_name)}",
                            arguments: "${escapeGraphQLString(args)}",
                            owner: "${escapeGraphQLString(ownerAddress)}",
                            type: "data"
                          ){
                            ownerAddress
                            contractAddress
                            contractName
                          }
                        }
                        `;

                        const deployContractResponse = await fetch(decryptedUrl, {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({ query: deployContractMutation }),
                        });

                        if (!deployContractResponse.ok) {
                            throw new Error(`Network response was not ok: ${deployContractResponse.statusText}`);
                        }

                        const deployContractResult = await deployContractResponse.json();
                        if (deployContractResult.errors) {
                            console.error('GraphQL errors in deployContract:', deployContractResult.errors);
                            sendResponse({ success: false, error: 'Error in deployContract mutation.', errors: deployContractResult.errors });
                            return;
                        }

                        // Extract the contract address and return success
                        if (deployContractResult.data && deployContractResult.data.deployContract && deployContractResult.data.deployContract.contractAddress) {
                            const contractAddress = deployContractResult.data.deployContract.contractAddress;
                            sendResponse({ success: true, contractAddress: contractAddress });
                            return;
                        } else {
                            sendResponse({ success: false, error: 'Failed to deploy contract.' });
                            return;
                        }

                    } else {
                        sendResponse({ success: false, error: 'Failed to add address.' });
                        return;
                    }

                } catch (error) {
                    console.error('Error deploying contract chain:', error);
                    sendResponse({ success: false, error: error.message });
                }
            });
        })();

        return true; // Keep the message channel open for async sendResponse
    }
});
