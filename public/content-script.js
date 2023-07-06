/*global chrome*/
chrome.runtime.onMessage.addListener(function(request, sender, sendResponse) {
  // Send a message to the webpage
  sendMessageToPage(request);
  sendResponse(true);
});

// Function to send a message to the webpage
function sendMessageToPage(request) {
  // Send a message to the webpage
  window.postMessage({ type: 'FROM_CONTENT_SCRIPT', data: request }, '*');
}

window.addEventListener("message", (event) => {
  if (
      event.source === window &&
      event.data.direction === "commit-page-script"
  ) {
      chrome.runtime.sendMessage({
      from: 'commit',
      data: event.data.message,
      amount: event.data.amount,
      address: event.data.address,
      event: event.data.event
      })
  }
  else if (event.source === window &&
      event.data.direction === "get-page-script") {
      chrome.runtime.sendMessage({
          from: 'get',
          id: event.data.id
      })
  }
});

const commit = document.getElementById('commit-page-script');
const get = document.getElementById('get-page-script');

if (commit !== null) {
  commit.addEventListener('click', function() {
    chrome.runtime.sendMessage({ action: 'showExtension' });
  });
}

if (get !== null){
  get.addEventListener('click', function() {
    chrome.runtime.sendMessage({ action: 'showExtension' });
  });
}