/*global chrome*/
chrome.runtime.onInstalled.addListener(function() {
    chrome.declarativeContent.onPageChanged.removeRules(undefined, function() {
      chrome.declarativeContent.onPageChanged.addRules([{
        conditions: [new chrome.declarativeContent.PageStateMatcher({
          pageUrl: { hostContains: '' }
        })
        ],
        actions: [new chrome.declarativeContent.ShowPageAction()]
      }]);
    });
});

chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
    if (request.action === 'showExtension') {
        chrome.storage.local.get("popupShown", function(data) {
        if (!data.popupShown) {
            chrome.windows.getCurrent(function(currentWindow) {      
                // Show the page action popup at the same position as the extension button
                chrome.windows.create({
                  url: chrome.extension.getURL("index.html"),
                  type: "popup",
                  top: 65,
                  left: 840,
                  height: 558,
                  width: 385
                });
            });
            // Save popupShown flag in storage
            chrome.storage.local.set({ "popupShown": true });
        }
    });
    }
});

// Reset popupShown flag when popup is closed
chrome.windows.onRemoved.addListener(function() {
    chrome.storage.local.set({ "popupShown": false });
});
