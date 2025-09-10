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



/*global chrome*/

// Function to get full hostname from URL
function getBaseDomain(url) {
  try {
    const urlObj = new URL(url);
    return urlObj.hostname;
  } catch (error) {
    return '';
  }
}

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  sendMessageToPage(request);
  sendResponse(true);
});

// Function to send a message to the webpage
function sendMessageToPage(request) {
  window.postMessage({ type: 'FROM_CONTENT_SCRIPT', data: request }, '*');
}

// Add event listener to listen for messages from the web page
window.addEventListener('message', (event) => {
  if (event.source === window) {
    const { direction } = event.data;
    if (direction === 'commit') {
      handleCommitOperation(event);
    } else if (direction === 'login') {
      handleLoginOperation(event);
    } else if (direction === 'custom') {
      handleCustomOperation(event);
    }
  }
});

// Handle commit operation and display the modal
function handleCommitOperation(event) {
  const { amount, data, recipient, styles } = event.data;

  // Ensure the amount is present before proceeding
  if (amount.trim() !== '') {
    const modal = createOrUpdateModal('COMMIT', amount, { amount, data, recipient }, styles);
    // Event delegation handles event listeners
  }
}

// Handle custom operation and display the modal
function handleCustomOperation(event) {
  const { data, recipient, styles, customMessage } = event.data;
  const amount = '1'; // Automatically assigned value 1
  const transactionData = { amount, data, recipient };
  const modal = createOrUpdateModal('', amount, transactionData, styles, true, customMessage || '');
  // Event delegation handles event listeners
}

// Handle login operation and display the login modal
function handleLoginOperation(event) {
  // Create or update the login modal
  const modal = createOrUpdateLoginModal();
  // Event delegation handles event listeners
}

// Create or update the login modal
function createOrUpdateLoginModal() {
  let modal = document.getElementById('resVaultLoginModal');
  const modalContent = generateLoginModalContent();

  if (!modal) {
    // Create the modal if it doesn't exist
    modal = document.createElement('div');
    modal.id = 'resVaultLoginModal';
    modal.style.cssText = `
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background-color: rgba(0, 0, 0, 0.5);
      z-index: 1000;
      display: block;
    `;
    modal.innerHTML = modalContent;
    document.body.appendChild(modal);

    // Attach a single event listener to the modal for event delegation
    modal.addEventListener('click', async (event) => {
      if (event.target.id === 'resVaultLoginModalClose') {
        modal.style.display = 'none';
      } else if (event.target.id === 'resVaultLoginModalAuthenticate') {
        const isConnected = await checkConnectionStatus();
        if (isConnected) {
          handleLoginTransactionSubmit();
        } else {
          sendMessageToPage('error');
        }
        modal.style.display = 'none';
      }
    });
  } else {
    // Update the modal content if it already exists
    modal.innerHTML = modalContent;
    modal.style.display = 'block'; // Ensure the modal is visible again
  }

  return modal;
}

// Generate the HTML content for the login modal
function generateLoginModalContent() {
  return `
    <div id="resVaultLoginModalContent" style="
      position: fixed; 
      top: 50%; 
      left: 50%; 
      transform: translate(-50%, -50%); 
      background-color: #0f0638; 
      padding: 20px; 
      border-radius: 10px; 
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      z-index: 1001;
      font-family: Poppins, sans-serif;
      width: 300px;
    ">
      <div style="width: calc(100%);
        padding: 20px;
        background-color: #291f57;
        border-radius: 15px;
        margin-bottom: 20px;
        box-shadow: 5px 5px 35px -14px rgba(0,0,0,.17); 
        text-align: center;">
        <p style="
          color: #fff; 
          font-size: 14px; 
          font-weight: bold;
          margin: 0;
        ">
          Authenticate with Res<span style="color: #47e7ce;">Vault</span>
        </p>
      </div>

      <div style="display: flex; justify-content: space-between;">
        <button id="resVaultLoginModalClose" style="
          background-color: #291f57;
          border: 1px #47e7ce solid;
          color: white; 
          border: none; 
          padding: 10px 20px; 
          border-radius: 5px; 
          cursor: pointer;
          width: 50%;
          box-sizing: border-box;
          margin-right: 5px;">
          Close
        </button>
        <button id="resVaultLoginModalAuthenticate" style="
          background: linear-gradient(60deg, #47e7ce, #4fa8c4); 
          color: white; 
          border: none; 
          padding: 10px 20px; 
          border-radius: 5px; 
          cursor: pointer;
          width: 50%;
          box-sizing: border-box;">
          Authenticate
        </button>
      </div>
    </div>
  `;
}

// Check if the user is connected to the website and net
function checkConnectionStatus() {
  return new Promise((resolve) => {
    const domain = window.location.hostname;

    chrome.storage.local.get(['keys', 'connectedNets'], (result) => {
      const keys = result.keys || {};
      const connectedNets = result.connectedNets || {};

      const net = connectedNets[domain];
      if (net && keys[domain] && keys[domain][net]) {
        resolve(true);
      } else {
        resolve(false);
      }
    });
  });
}

// Handle login transaction submission and send data to background script
function handleLoginTransactionSubmit() {
  chrome.runtime.sendMessage(
    {
      action: 'submitLoginTransaction',
    },
    (response) => {
      if (response) {
        // Send the response to the page script
        window.postMessage({ type: 'FROM_CONTENT_SCRIPT', data: response }, '*');
      }
    }
  );
}

// Create or update the modal with the necessary transaction details
function createOrUpdateModal(operation, amount, transactionData, styles = {}, isCustom = false, customMessage = '') {
  let modal = document.getElementById('resVaultModal');
  const modalContent = generateModalContent(operation, amount, styles, isCustom, customMessage);

  if (!modal) {
    // Create the modal if it doesn't exist
    modal = document.createElement('div');
    modal.id = 'resVaultModal';
    modal.style.cssText = `
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background-color: rgba(0, 0, 0, 0.5);
      z-index: 1000;
      display: block;
    `;
    modal.innerHTML = modalContent;
    document.body.appendChild(modal);

    // Attach a single event listener to the modal for event delegation
    modal.addEventListener('click', (event) => {
      if (event.target.id === 'resVaultModalClose') {
        modal.style.display = 'none';
      } else if (event.target.id === 'resVaultModalSubmit') {
        handleTransactionSubmit(modal.transactionData); // Use modal.transactionData
        modal.style.display = 'none';
      }
    });
  } else {
    // Update the modal content if it already exists
    modal.innerHTML = modalContent;
    modal.style.display = 'block'; // Ensure the modal is visible again
  }

  // Update transactionData on the modal element
  modal.transactionData = transactionData;

  // Apply user styles if any
  applyStylesToModal(modal, styles);

  return modal;
}

// Generate the HTML content for the commit or custom modal
function generateModalContent(operation, amount, styles = {}, isCustom = false, customMessage = '') {
  if (isCustom) {
    return `
    <div id="resVaultModalContent" style="
      position: fixed; 
      top: 50%; 
      left: 50%; 
      transform: translate(-50%, -50%); 
      background-color: #0f0638; 
      padding: 20px; 
      border-radius: 10px; 
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      z-index: 1001;
      font-family: Poppins, sans-serif;
      width: 300px;
    ">
      <div id="resVaultMessageBox" style="width: calc(100%);
        padding: 20px;
        background-color: #291f57;
        border-radius: 15px;
        margin-bottom: 20px;
        box-shadow: 5px 5px 35px -14px rgba(0,0,0,.17);">
        <form id="Form" style="margin: auto;">
          <div style="width: 100%; margin-bottom: 10px;">
            <div id="customMessage" style="padding: 0;
              width: 100%;
              margin: 0;
              overflow: hidden;
              border-radius: 0px;
              color: #fff;
              text-align: center;
            ">
              <p>
                ${customMessage || ''}
              </p>
            </div>
          </div>
          <p id="poweredBy" style="font-size: small; color: #fff">Powered by Res<strong style="color: #47e7ce">Vault</strong></p>
        </form>
      </div>
      <span style="display: flex;">
          <button id="resVaultModalClose" style="
            background-color: #291f57;
            border: none;
            color: white; 
            padding: 10px 20px; 
            border-radius: 5px; 
            cursor: pointer;
            width: 50%;
            box-sizing: border-box;
            margin-right: 5px;">Cancel</button>
          <button id="resVaultModalSubmit" style="
            background: linear-gradient(60deg, #47e7ce, #4fa8c4); 
            color: white; 
            border: none; 
            padding: 10px 20px; 
            border-radius: 5px; 
            cursor: pointer;
            width: 50%;
            box-sizing: border-box;">Submit</button>
      </span>                       
    </div>`;
  } else {
    return `
    <div id="resVaultModalContent" style="
      position: fixed; 
      top: 50%; 
      left: 50%; 
      transform: translate(-50%, -50%); 
      background-color: #0f0638; 
      padding: 20px; 
      border-radius: 10px; 
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      z-index: 1001;
      font-family: Poppins, sans-serif;
      width: 300px;
    ">
      <div id="resVaultMessageBox" style="width: calc(100%);
        padding: 20px;
        background-color: #291f57;
        border-radius: 15px;
        margin-bottom: 20px;
        box-shadow: 5px 5px 35px -14px rgba(0,0,0,.17);">
        <form id="Form" style="margin: auto;">
          <div style="width: 100%; margin-bottom: 10px;">
            <div style="padding: 0;
              width: 100%;
              margin: 0;
              overflow: hidden;
              border-radius: 0px;
              color: #fff;
              border-bottom: 1px rgba(255, 255, 255, 0.3) solid;">
              <p>
                Operation: ${operation}
              </p>
            </div>
          </div>
          <div style="width: 100%;
            margin-bottom: 10px; display: flex; align-items: center; justify-content: space-between;">
            <p id="amountDisplay" style="width: calc(((100% / 3) * 2) - 35px);
              border: none;
              background-color: transparent;
              padding: 10px 0px;
              border-radius: 0px;
              border-bottom: 1px rgba(255, 255, 255, 0.3) solid;
              color: #fff;
              text-align: center;
              ">
              ${amount}
            </p>
            <div style="display: flex;
              align-items: center;">
              <div style="display: block; font-family: Arial, Helvetica, sans-serif; font-size: 20px; font-weight: bold; color: #f0f0f0; background-color: #808080; border-radius: 50%; width: 30px; height: 30px; display: flex; align-items: center; justify-content: center; margin: 10px; transform: rotate(20deg);">R</div>
              <span style="color: #fff; font-weight: 600; padding-left: 5px;">RoK</span>
            </div>
          </div>
          <p style="font-size: small; color: #fff">Powered by Res<strong style="color: #47e7ce">Vault</strong></p>
        </form>
      </div>
      <span style="display: flex;">
          <button id="resVaultModalClose" style="
            background-color: #291f57;
            border: none;
            color: white; 
            padding: 10px 20px; 
            border-radius: 5px; 
            cursor: pointer;
            width: 50%;
            box-sizing: border-box;
            margin-right: 5px;">Cancel</button>
          <button id="resVaultModalSubmit" style="
            background: linear-gradient(60deg, #47e7ce, #4fa8c4); 
            color: white; 
            border: none; 
            padding: 10px 20px; 
            border-radius: 5px; 
            cursor: pointer;
            width: 50%;
            box-sizing: border-box;">Submit</button>
      </span>                       
    </div>`;
  }
}

// Apply user styles to the modal elements
function applyStylesToModal(modal, styles) {
  const allowedSelectors = [
    '#resVaultModalContent',
    "#resVaultMessageBox",
    '#amountDisplay',
    '#resVaultModalClose',
    '#resVaultModalSubmit',
    '#customMessage',
    '#poweredBy',
  ];

  const allowedStyles = {
    '#resVaultModalContent': ['background-color', 'padding', 'border-radius', 'box-shadow', 'font-family', 'width', 'position', 'top', 'left', 'transform', 'z-index'],
    '#amountDisplay': ['color', 'font-size', 'text-align', 'border', 'background-color', 'padding', 'border-radius'],
    '#resVaultMessageBox': ['width', 'padding', 'background-color', 'border-radius', 'margin-bottom', 'box-shadow'],
    '#resVaultModalClose': ['background-color', 'color', 'border', 'padding', 'border-radius', 'cursor', 'width'],
    '#resVaultModalSubmit': ['background-color', 'color', 'border', 'padding', 'border-radius', 'cursor', 'width'],
    '#customMessage': ['color', 'font-size', 'text-align', 'background-color', 'padding', 'border-radius'],
    '#poweredBy': ['color', 'font-size', 'text-align'],
  };

  for (const selector in styles) {
    if (!allowedSelectors.includes(selector)) {
      continue; // Skip disallowed selectors
    }
    const elements = modal.querySelectorAll(selector);
    elements.forEach(element => {
      const styleObj = styles[selector];
      const allowedProperties = allowedStyles[selector] || [];
      for (const styleName in styleObj) {
        if (allowedProperties.includes(styleName)) {
          // Apply the style with !important to ensure precedence
          element.style.setProperty(styleName, styleObj[styleName], 'important');
        }
      }
    });
  }
}

// Handle transaction submission and send data to background script
function handleTransactionSubmit({ amount, data, recipient }) {
  chrome.runtime.sendMessage(
    {
      action: 'submitTransaction',
      amount,
      data,
      recipient,
    },
    (response) => {
      if (response) {
        // Send the response to the page script
        window.postMessage({ type: 'FROM_CONTENT_SCRIPT', data: response }, '*');
      }
    }
  );
}
