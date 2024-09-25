// background.js

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
    } else if (request.action === 'submitTransaction') {
        (async function() {
            const senderUrl = sender.tab ? sender.tab.url : sender.url || null;
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

                        // Log the mutation for debugging
                        console.log('Mutation:', mutation);

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
    }
});