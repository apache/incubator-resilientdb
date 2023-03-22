/*global chrome*/
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

document.getElementById('commit-page-script').addEventListener('click', function() {
  chrome.runtime.sendMessage({ action: 'showExtension' });
});

document.getElementById('get-page-script').addEventListener('click', function() {
  chrome.runtime.sendMessage({ action: 'showExtension' });
});