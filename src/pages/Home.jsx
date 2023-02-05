import logo from '../logo.png';
import '../App.css';
import React, { useState } from 'react';
import CryptoJS from "crypto-js";
import { sendRequest } from '../client';
import Footer from "../components/Footer";
import { useAlert } from 'react-alert'

function Home(props) {
  const alert = useAlert();

  const createAccount = async () => {
    if(props.password===props.confirmPassword){
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
        props.password
      ).toString();
      console.log(props.publicKey);
      console.log(phrase);

      const bytes = CryptoJS.AES.decrypt(phrase, props.password);
      const data = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
      console.log(data);
    }
    else {
      alert.show("Passwords don't match.");
    }
    
  }

  const onChangePasswordHandler = event => {
    props.setPassword(event.target.value);
  };

  const onChangeConfirmPasswordHandler = event => {
    props.setConfirmPassword(event.target.value);
  };
 

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

      <div className="paymentTop vcenter">
          <input type="password" value={props.password} onChange={onChangePasswordHandler} placeholder="Enter Password" />
      </div>
      <div className="paymentBottom vcenter">
          <input type="password" value={props.confirmPassword} onChange={onChangeConfirmPasswordHandler} placeholder="Confirm Password" />
      </div>
    
      <div className="payment vcenter">
        <button className="buttonCreate center" onClick={createAccount}>Create Account</button>
      </div>

      
      {props.publicKeyDisplay &&
      <div>
      </div>
      }

      <Footer />
    </div>
  </div>
  )
}

export default Home;