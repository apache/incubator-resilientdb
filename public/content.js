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
    if (event.data.direction === 'commit') {
      handleCommitOperation(event);
    } else if (event.data.direction === 'login') {
      handleLoginOperation(event);
    }
  }
});

// Handle commit operation and display the modal
function handleCommitOperation(event) {
  const { amount, data, recipient } = event.data;

  // Ensure the amount is present before proceeding
  if (amount.trim() !== '') {
    const modal = createOrUpdateModal(amount, { amount, data, recipient });
    // Event delegation handles event listeners
  }
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
          alert('Please connect to the website and net in ResVault extension.');
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

// Existing functions for commit operation remain unchanged

// Create or update the modal with the necessary transaction details
function createOrUpdateModal(amount, transactionData) {
  let modal = document.getElementById('resVaultModal');
  const modalContent = generateModalContent('COMMIT', amount);

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

  return modal;
}

// Generate the HTML content for the commit modal
function generateModalContent(operation, amount) {
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
    <div style="width: calc(100%);
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