/*global chrome*/
window.addEventListener("message", (event) => {
  if (
    event.source === window &&
    event.data.direction === "from-page-script"
  ) {
    chrome.runtime.sendMessage({
      from: 'content',
      data: event.data.message,
      amount: event.data.amount,
      address: event.data.address,
      event: event.data.event
    })
  }
  else if (event.source === window &&
    event.data.direction === "id-page-script") {
      chrome.runtime.sendMessage({
        from: 'fetch',
        id: event.data.id
      })
    }
});