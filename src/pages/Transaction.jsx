/*global chrome*/
import logo from '../logo.png';
import '../App.css';
import Footer from "../components/Footer";
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";
import { sendRequest } from '../client';
import React, { useEffect } from 'react';

function Transaction(props) {
  const location = useLocation();
  props.setFooter("footerLogin");
  
  const submit = async () => {
    const port = chrome.runtime.connect();
    if ((location.state.from === 'commit')) {
      var escapeCodes = { 
          '\\': '\\',
          'r':  '\r',
          'n':  '\n',
          't':  '\t'
      };
      location.state.data.replace(/\\(.)/g, function(str, char) {
        return escapeCodes[char];
      });

      const query = `mutation {
        postTransaction(data: {
          operation: "CREATE",
          amount: ${parseInt(location.state.amount)},
          signerPublicKey: "${location.state.publicKey}",
          signerPrivateKey: "${location.state.privateKey}",
          recipientPublicKey: "${location.state.address}",
          asset: """{
              "data": { 
                ${location.state.data}
              },
          }
          """
        }){
          id
        }
      }`

      const result = sendRequest(query).then(res => { 
        const store = location.state;
        store.id = res.data.postTransaction.id;
        chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
          // Send a message to the content script
          chrome.tabs.sendMessage(tabs[0].id, store.id)
        });
        props.navigate("/logs", { state: store });
      });
    }

    else if ((location.state.from === 'get')){
      const query = `query {
        getTransaction(id: "${location.state.id}"){
          id
          version
          amount
          metadata
          operation
          asset
          publicKey
          uri
          type
        }
      }`

      const result = sendRequest(query).then(res => { 
        const store = location.state;
        store.id = res.data.getTransaction.id;
        chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
          // Send a message to the content script
          chrome.tabs.sendMessage(tabs[0].id, res.data.getTransaction)
        });
        props.navigate("/logs", {state: store});
      });
    }
  }

  const back = async () => {
    const store = location.state;
    chrome.storage.local.clear(function(){
      props.navigate("/login", {state: store});
    });
  }
 
  return (
  <div className="App">
    <div className="mainContainer">
      <div className="cardHolder">
        <div className="header">
          <div className="heading center">
            Global-Scale Blockchain Fabric
          </div> 
          <div className="stepHeading center">NexRes Wallet</div>
          <div className="logo">
            <img src={logo} alt="logo" />
          </div>
          <div className="paymentTopKey vcenter">
            <p className="publicKeyStyle" style={{color: 'white'}}><b>Account:</b> {location.state.publicKey}</p>
            <button className="buttonSignOut" data-inline="true" onClick={back}><ExitToAppIcon /></button>
          </div>
        </div>
      </div>

      <div className="paymentBottomDashboard vcenter">
      {location.state.from === 'commit' ? (
        <p className="manifest">
          <ul className="list">
            <li className="list"><span className="list">Operation: {location.state.operation} </span></li>
            <li className="list"><span className="list">Your Account: {location.state.publicKey} </span></li>
            <li className="list"><span className="list">Recipient Account: {location.state.address} </span></li>
            <li className="list"><span className="list">Amount: {location.state.amount} </span></li>
          </ul>
        </p>
      ):(
        <p className="manifest">
          <ul className="list">
            <li className="list"><span className="list">Your Account: {location.state.publicKey}</span></li>
            <li className="list"><span className="list">Transaction ID: {location.state.id}</span></li>
            </ul>
        </p>
      )}
      </div>

      <div className="paymentBottomDashboardBack vcenter">
        <button className="buttonCreate center" data-inline="true" onClick={submit}> Submit </button>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Transaction;