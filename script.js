import ResilientSDK from 'https://cdn.resilientdb.com/resilient-sdk.js';

const sdk = new ResilientSDK();

// Add a message listener
sdk.addMessageListener((event) => {
    const message = event.data.data;
    alert(JSON.stringify(message));  // Set the message
});

var commit = document.querySelector('[data-nexres="commit-page-script"]');
var fetcher = document.querySelector('[data-nexres="get-page-script"]');
var update = document.querySelector('[data-nexres="update-page-script"]');
var updateMulti = document.querySelector('[data-nexres="update-multi-page-script"]');
var filter = document.querySelector('[data-nexres="filter-page-script"]');
var account = document.querySelector('[data-nexres="account-page-script"]');
var data = document.querySelector('[data-nexres="get-data"]');
var amount = document.querySelector('[data-nexres="get-amount"]');
var address = document.querySelector('[data-nexres="get-address"]');
var id = document.querySelector('[data-nexres="get-id"]');
var updateId = document.querySelector('[data-nexres="update-id"]');
var updateData = document.querySelector('[data-nexres="update-data"]');
var updateAmount = document.querySelector('[data-nexres="update-amount"]');
var updateAddress = document.querySelector('[data-nexres="update-address"]');
var ownerPublicKey = document.querySelector('[data-nexres="filter-owner-key"]');
var recipientPublicKey = document.querySelector('[data-nexres="filter-recipient-key"]');
var updateMultiId1 = document.querySelector('[data-nexres="update-multi-id1"]');
var updateMultiData1 = document.querySelector('[data-nexres="update-multi-data1"]');
var updateMultiAmount1 = document.querySelector('[data-nexres="update-multi-amount1"]');
var updateMultiAddress1 = document.querySelector('[data-nexres="update-multi-address1"]');
var updateMultiId2 = document.querySelector('[data-nexres="update-multi-id2"]');
var updateMultiData2 = document.querySelector('[data-nexres="update-multi-data2"]');
var updateMultiAmount2 = document.querySelector('[data-nexres="update-multi-amount2"]');
var updateMultiAddress2 = document.querySelector('[data-nexres="update-multi-address2"]');

commit.addEventListener("click", commitContentScript);
fetcher.addEventListener("click", fetchContentScript);
update.addEventListener("click", updateContentScript);
updateMulti.addEventListener("click", updateMultiContentScript);
filter.addEventListener("click", filterContentScript);
account.addEventListener("click", accountContentScript);

function commitContentScript() {
    sdk.sendMessage({
      direction: "commit-page-script",
      message: data.value,
      amount: amount.value,
      address: address.value
    });
}

function fetchContentScript() {
    sdk.sendMessage({
      direction: "get-page-script",
      id: id.value
    });
}

function updateContentScript() {
    sdk.sendMessage({
      direction: "update-page-script",
      id: updateId.value,
      message: updateData.value,
      amount: updateAmount.value,
      address: updateAddress.value
    });
}

function updateMultiContentScript() {
    const valuesList = [
      {
        id: updateMultiId1.value,
        message: updateMultiData1.value,
        amount: updateMultiAmount1.value,
        address: updateMultiAddress1.value,
      },
      {
        id: updateMultiId2.value,
        message: updateMultiData2.value,
        amount: updateMultiAmount2.value,
        address: updateMultiAddress2.value,
      }
    ];

    sdk.sendMessage({
      direction: "update-multi-page-script",
      values: valuesList
    });
}

function filterContentScript() {
    sdk.sendMessage({
      direction: "filter-page-script",
      owner: ownerPublicKey.value,
      recipient: recipientPublicKey.value,
    });
}

function accountContentScript() {
    sdk.sendMessage({
      direction: "account-page-script",
    });
}
