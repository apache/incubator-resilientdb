/*global chrome*/
/*
chrome.runtime.onInstalled.addListener(function() {
  chrome.declarativeContent.onPageChanged.removeRules(undefined, function() {
    chrome.declarativeContent.onPageChanged.addRules([
      {
        conditions: [
          new chrome.declarativeContent.PageStateMatcher({
            pageUrl: { hostContains: '' }
          })
        ],
        actions: [new chrome.declarativeContent.ShowPageAction()]
      }
    ]);
  });
});

chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
    console.log("Message received:", request);
    if (request.action === 'showExtension') {
      console.log("Action: showExtension");

      // Perform asynchronous operation
      performAsyncOperation(request)
        .then(response => {
          console.log("Async operation complete. Sending response:", response);
          sendResponse(response);
        })
        .catch(error => {
          console.error("Error in performing async operation:", error);
          sendResponse({ error: error });
        });
    }
    // Return true to indicate that sendResponse will be called asynchronously
    return true;
  });

// Reset popupShown flag when popup is closed
chrome.windows.onRemoved.addListener(function() {
  chrome.storage.local.set({ "popupShown": false });
});

// Perform asynchronous operation for showExtension action
function performAsyncOperation(request) {
  return new Promise((resolve, reject) => {
    chrome.storage.local.get("popupShown", function(data) {
      if (!data.popupShown) {
        chrome.windows.getCurrent(function(currentWindow) {
          // Show the page action popup at the same position as the extension button
          chrome.windows.create(
            {
              url: "index.html",
              type: "popup",
              top: 65,
              left: 840,
              height: 558,
              width: 385
            },
            function(window) {
              // Save popupShown flag in storage
              chrome.storage.local.set({ "popupShown": true });
              resolve({ popupShown: true });
            }
          );
        });
      } else {
        resolve({ popupShown: false });
      }
    });
  });
}*/
