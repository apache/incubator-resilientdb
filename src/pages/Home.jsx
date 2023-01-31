import logo from '../logo.png';
import '../App.css';
import React, { useState } from 'react';
import CryptoJS from "crypto-js";
import { sendRequest } from '../client';
import Footer from "../components/Footer";

function Home(props) {
  const createAccount = async () => {
    const query = `mutation {
        generateKeys {
          publicKey
          privateKey
        }
      }`
    
    const res = await sendRequest(query);
    const getPublicKey = res.data.generateKeys.publicKey;
    props.setPublicKeyDisplay(true);
    props.setPublicKey(getPublicKey);
    const phrase = CryptoJS.AES.encrypt(
      JSON.stringify(res.data.generateKeys.privateKey),
      "abc1234"
    ).toString();
    console.log(phrase);

    const bytes = CryptoJS.AES.decrypt(phrase, "abc1234");
    const data = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
    console.log(data);
  }

  return (
  <div className="App">
    <div className="mainContainer">
      <div className="cardHolder">
        <div className="header">
          <div className="heading center">Global-Scale Blockchain Fabric</div>
          <div className="stepHeading center">NexRes Wallet</div>
          <div className="logo">
            <img src={logo} alt="logo" />
          </div>
        </div>
      </div>
    
      <div className="payment vcenter">
        <button className="buttonCreate center" onClick={createAccount}>Create Account</button>
      </div>

      
      {props.publicKeyDisplay &&
      <div>
        <div className='paymentNew vcenter'> 
          <div>Public Key</div> 
        </div> 
        <div className='paymentKey vcenter'> 
          <div className='publicKey'>{props.publicKey}</div>
        </div>
      </div>
      }

      <Footer />
    </div>
  </div>
  )
}

export default Home;